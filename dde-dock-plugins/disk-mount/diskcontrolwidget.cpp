/*
 * Copyright (C) 2016 ~ 2018 Deepin Technology Co., Ltd.
 *               2016 ~ 2018 dragondjf
 *
 * Author:     dragondjf<dingjiangfeng@deepin.com>
 *
 * Maintainer: dragondjf<dingjiangfeng@deepin.com>
 *             zccrs<zhangjide@deepin.com>
 *             Tangtong<tangtong@deepin.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "diskcontrolwidget.h"
#include "diskcontrolitem.h"

#include <dfmdiskmanager.h>
#include <dfmblockdevice.h>
#include <dfmdiskdevice.h>
#include <dfmsettings.h>
#include <DDesktopServices>

#include <QDebug>
#include <QProcess>
#include <QThreadPool>
#include <QtConcurrent>
#include <QScrollBar>
#include <QDebug>

#define WIDTH           300

DWIDGET_USE_NAMESPACE

DFM_USE_NAMESPACE

DiskControlWidget::DiskControlWidget(QWidget *parent)
    : QScrollArea(parent),
      m_centralLayout(new QVBoxLayout),
      m_centralWidget(new QWidget)
{
    m_centralWidget->setLayout(m_centralLayout);
    m_centralWidget->setFixedWidth(WIDTH);

    setWidget(m_centralWidget);
    setFixedWidth(WIDTH);
    setFrameStyle(QFrame::NoFrame);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    verticalScrollBar()->setSingleStep(7);
    setStyleSheet("background-color:transparent;");
    m_diskManager = new DFMDiskManager(this);
    initConnect();
}

void DiskControlWidget::initConnect()
{
    connect(m_diskManager, &DFMDiskManager::diskDeviceAdded, this, [this](const QString &path) {
        // blumia: Workaround. Wait for udisks2 add new device to device list.
        QTimer::singleShot(500, this, [=](){
            onDriveConnected(path);
        });
    });
    connect(m_diskManager, &DFMDiskManager::diskDeviceRemoved, this, &DiskControlWidget::onDriveDisconnected);
    connect(m_diskManager, &DFMDiskManager::mountAdded, this, &DiskControlWidget::onMountAdded);
    connect(m_diskManager, &DFMDiskManager::mountRemoved, this, &DiskControlWidget::onMountRemoved);
    connect(m_diskManager, &DFMDiskManager::fileSystemAdded, this, &DiskControlWidget::onVolumeAdded);
    connect(m_diskManager, &DFMDiskManager::fileSystemRemoved, this, &DiskControlWidget::onVolumeRemoved);
}

void DiskControlWidget::startMonitor()
{
    m_diskManager->setWatchChanges(true);
    onDiskListChanged();
}

void DiskControlWidget::doStartupAutoMount()
{
    if (getGsGlobal()->value("GenericAttribute", "AutoMount", false).toBool() == false) {
        return;
    }

    QStringList blDevList = m_diskManager->blockDevices();
    for (const QString& blDevStr : blDevList) {
        QScopedPointer<DFMBlockDevice> blDev(DFMDiskManager::createBlockDevice(blDevStr));

        if (blDev->isEncrypted()) continue;
        if (blDev->hintIgnore()) continue;

        if (blDev->hasFileSystem() && blDev->mountPoints().isEmpty()) {
            blDev->mount({{"auth.no_user_interaction", true}});
        }
    }
}

void DiskControlWidget::unmountAll()
{
    QStringList blockDevices = m_diskManager->blockDevices();

    for (const QString & blDevStr : blockDevices) {
        QScopedPointer<DFMBlockDevice> blDev(DFMDiskManager::createBlockDevice(blDevStr));
        if (blDev->hasFileSystem() /* && DFMSetting*/ && !blDev->mountPoints().isEmpty()) {
            QByteArray mountPoint = blDev->mountPoints().first();
            if (mountPoint != QStringLiteral("/boot") && mountPoint != QStringLiteral("/") && mountPoint != QStringLiteral("/home")) {
                blDev->unmount({});
            }
        }
    }
}

void DiskControlWidget::onDiskListChanged()
{
    while (QLayoutItem *item = m_centralLayout->takeAt(0)) {
        delete item->widget();
        delete item;
    }

    int mountedCount = 0;
    QStringList blDevList = m_diskManager->blockDevices();
    for (const QString& blDevStr : blDevList) {
        QScopedPointer<DFMBlockDevice> blDev(DFMDiskManager::createBlockDevice(blDevStr));
        if (blDev->hasFileSystem() && !blDev->mountPoints().isEmpty() && !blDev->hintIgnore()) {
            QByteArray mountPoint = blDev->mountPoints().first();
            if (mountPoint != QStringLiteral("/boot") && mountPoint != QStringLiteral("/") && mountPoint != QStringLiteral("/home")) {
                mountedCount++;
                DiskControlItem *item = new DiskControlItem(blDev.data(), this);
                connect(item, &DiskControlItem::requestUnmount, this, &DiskControlWidget::unmountDisk);
                m_centralLayout->addWidget(item);
            }
        }
    }

    emit diskCountChanged(mountedCount);

    const int contentHeight = mountedCount * 70;
    const int maxHeight = std::min(contentHeight, 70 * 6);

    m_centralWidget->setFixedHeight(contentHeight);
    setFixedHeight(maxHeight);

    verticalScrollBar()->setPageStep(maxHeight);
    verticalScrollBar()->setMaximum(contentHeight - maxHeight);
}

void DiskControlWidget::onDriveConnected(const QString &deviceId)
{
    QScopedPointer<DFMDiskDevice> diskDevice(DFMDiskManager::createDiskDevice(deviceId));
    if (diskDevice->removable()) {
        DDesktopServices::playSystemSoundEffect("device-added");

        bool mountAndOpen = false;

        // Check if we need do auto mount..
        getGsGlobal()->reload();
        if (getGsGlobal()->value("GenericAttribute", "AutoMountAndOpen", false).toBool()) {
            // mount and open
            mountAndOpen = true;
        } else if (getGsGlobal()->value("GenericAttribute", "AutoMount", false).toBool()) {
            // mount
            // no flag there..
        } else {
            // no need to do auto mount, return.
            return;
        }

        // Do auto mount stuff..
        QStringList blDevList = m_diskManager->blockDevices();
        for (const QString& blDevStr : blDevList) {
            QScopedPointer<DFMBlockDevice> blDev(DFMDiskManager::createBlockDevice(blDevStr));

            if (blDev->drive() != deviceId) continue;
            if (blDev->isEncrypted()) continue;
            if (blDev->hintIgnore()) continue;

            if (blDev->hasFileSystem() && blDev->mountPoints().isEmpty()) {

                // blumia: if mount&open enabled and dde-file-manager also got installed, use dde-file-manager.
                //         using mount scheme with udisks sub-scheme to give user a *device is mounting* feedback.
                if (mountAndOpen && !QStandardPaths::findExecutable(QStringLiteral("dde-file-manager")).isEmpty()) {
                    QString mountUrlStr = "mount://fromMountPlugin#udisks://" + blDevStr;
                    QProcess::startDetached(QStringLiteral("dde-file-manager"), {mountUrlStr});
                    return;
                }

                QString mountPoint = blDev->mount({});
                if (mountAndOpen && !mountPoint.isEmpty()) {
                    DDesktopServices::showFolder(QUrl::fromLocalFile(mountPoint));
                }
            }
        }
    }
}

void DiskControlWidget::onDriveDisconnected()
{
    DDesktopServices::playSystemSoundEffect("device-removed");
}

void DiskControlWidget::onMountAdded()
{
    onDiskListChanged();
}

void DiskControlWidget::onMountRemoved()
{
    onDiskListChanged();
}

void DiskControlWidget::onVolumeAdded()
{
    onDiskListChanged();
}

void DiskControlWidget::onVolumeRemoved()
{
    onDiskListChanged();
}

void DiskControlWidget::onVolume_changed(const QDiskInfo &diskInfo)
{
    Q_UNUSED(diskInfo)
    onDiskListChanged();
}

void DiskControlWidget::unmountDisk(const QString &diskId) const
{
    QScopedPointer<DFMBlockDevice> blDev(DFMDiskManager::createBlockDevice(diskId));
    blDev->unmount({});
    if (blDev->device().startsWith("/dev/sr")) { // is a DVD driver
        QScopedPointer<DFMDiskDevice> diskDev(DFMDiskManager::createDiskDevice(blDev->drive()));
        if (diskDev->ejectable()) {
            diskDev->eject({});
        }
    }
}
