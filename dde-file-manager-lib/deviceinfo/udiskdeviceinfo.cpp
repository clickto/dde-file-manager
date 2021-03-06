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

#include "udiskdeviceinfo.h"
#include "dfmapplication.h"

#include "app/define.h"
#include "shutil/fileutils.h"
#include "utils.h"
#include "durl.h"
#include "udisklistener.h"
#include "singleton.h"
#include "gvfs/gvfsmountmanager.h"
#include "gvfs/qdrive.h"
#include "app/define.h"
#include "dfmdiskmanager.h"
#include "dfmblockdevice.h"

#include <QIcon>

#include <unistd.h>

DFM_USE_NAMESPACE

#if 0 // storage i10n
QT_TRANSLATE_NOOP3("DeepinStorage", "data", "Data Partition")
#endif

UDiskDeviceInfo::UDiskDeviceInfo()
    : DFileInfo("", false)
{

}

UDiskDeviceInfo::UDiskDeviceInfo(UDiskDeviceInfoPointer info)
    : UDiskDeviceInfo()
{
    setDiskInfo(info->getDiskInfo());
}

UDiskDeviceInfo::UDiskDeviceInfo(const DUrl &url)
    : DFileInfo(url, false)
{

}

UDiskDeviceInfo::UDiskDeviceInfo(const QString &url)
    : DFileInfo(url, false)
{

}

UDiskDeviceInfo::~UDiskDeviceInfo()
{

}

void UDiskDeviceInfo::setDiskInfo(QDiskInfo diskInfo)
{
    m_diskInfo = diskInfo;
    DUrl url = getMountPointUrl();

    url.setQuery(getMountPoint());
    setUrl(url);
}

QDiskInfo UDiskDeviceInfo::getDiskInfo() const
{
    return m_diskInfo;
}

QString UDiskDeviceInfo::getId() const
{
    return m_diskInfo.id();
}

QString UDiskDeviceInfo::getName() const
{
    QString letter = deviceListener->getVolumeLetters().value(m_diskInfo.uuid());
    if (!letter.isEmpty()) {
        return QString("%1 (%2:)").arg(m_diskInfo.name(), letter);
    }
    return m_diskInfo.name();
}

QString UDiskDeviceInfo::getType() const
{
    return m_diskInfo.type();
}

QString UDiskDeviceInfo::getPath() const
{
    return m_diskInfo.unix_device();
}

QString UDiskDeviceInfo::getMountPoint() const
{
    return m_diskInfo.mounted_root_uri();
}

DUrl UDiskDeviceInfo::getMountPointUrl() const
{
    if (!getId().isEmpty()) {
        return GvfsMountManager::getRealMountUrl(m_diskInfo);
    } else {
        return DUrl();
    }
}

QString UDiskDeviceInfo::getIcon() const
{
    return m_diskInfo.iconName();
}

bool UDiskDeviceInfo::canEject() const
{
    return m_diskInfo.can_eject();
}

bool UDiskDeviceInfo::canStop() const
{
    qDebug() << gvfsMountManager->Drives.contains(getDiskInfo().drive_unix_device()) << getDiskInfo().drive_unix_device();
    if (gvfsMountManager->Drives.contains(getDiskInfo().drive_unix_device())) {
        const QDrive &drive = gvfsMountManager->Drives.value(getDiskInfo().drive_unix_device());
        if (drive.start_stop_type() == G_DRIVE_START_STOP_TYPE_SHUTDOWN && drive.is_removable() &&  drive.can_stop()) {
            return true;
        }
    }
    return false;
}

bool UDiskDeviceInfo::canUnmount() const
{
    return m_diskInfo.can_unmount();
}

qulonglong UDiskDeviceInfo::getFree()
{
    //when device is mounted, use QStorageInfo to get datas
    if (canUnmount()) {
        if (getMediaType() == dvd || getMediaType() == native || getMediaType() == removable) {
            return QStorageInfo(getMountPointUrl().toLocalFile()).bytesFree();
        }
    }
    return m_diskInfo.free();
}

qulonglong UDiskDeviceInfo::getTotal()
{
    if (canUnmount()) {
        if (getMediaType() == dvd || getMediaType() == native || getMediaType() == removable) {
            return QStorageInfo(getMountPointUrl().toLocalFile()).bytesTotal();
        }
    }
    return m_diskInfo.total();
}

qint64 UDiskDeviceInfo::size() const
{
    return m_diskInfo.total();
}

QString UDiskDeviceInfo::fileDisplayName() const
{
    QString displayName = getName();
    if (!displayName.isEmpty()) {
        if (displayName.startsWith(ddeI18nSym)) {
            displayName = displayName.mid(ddeI18nSym.size(), displayName.size() - ddeI18nSym.size());
            return qApp->translate("DeepinStorage", displayName.toUtf8().constData());
        }
        return displayName;
    }
    return FileUtils::formatSize(size());
}

UDiskDeviceInfo::MediaType UDiskDeviceInfo::getMediaType() const
{
    if (getType() == "native") {
        return native;
    } else if (getType() == "removable") {
        return removable;
    } else if (getType() == "network") {
        return network;
    } else if (getType() == "smb") {
        return network;
    } else if (getType() == "phone") {
        return phone;
    } else if (getType() == "iphone") {
        return iphone;
    } else if (getType() == "camera") {
        return camera;
    } else if (getType() == "dvd") {
        return dvd;
    } else {
        return unknown;
    }
}

QString UDiskDeviceInfo::deviceTypeDisplayName() const
{
    if (getType() == "native") {
        return QObject::tr("Local disk");
    } else if (getType() == "removable") {
        return QObject::tr("Removable disk");
    } else if (getType() == "network") {
        return QObject::tr("Network shared directory");
    } else if (getType() == "phone") {
        return QObject::tr("Android mobile device");
    } else if (getType() == "iphone") {
        return QObject::tr("Apple mobile device");
    } else if (getType() == "camera") {
        return QObject::tr("Camera");
    } else if (getType() == "dvd") {
        return QObject::tr("Dvd");
    } else {
        return QObject::tr("Unknown device");
    }
}

QString UDiskDeviceInfo::sizeDisplayName() const
{
    if (filesCount() <= 1) {
        return QObject::tr("%1 item").arg(filesCount());
    } else {
        return QObject::tr("%1 items").arg(filesCount());
    }
}

int UDiskDeviceInfo::filesCount() const
{
    return FileUtils::filesCount(const_cast<UDiskDeviceInfo *>(this)->getMountPointUrl().toLocalFile());
}

bool UDiskDeviceInfo::isReadable() const
{
    return true;
}

bool UDiskDeviceInfo::isWritable() const
{
    return true;
}

bool UDiskDeviceInfo::canRename() const
{
    // blumia: since we now both use the old gvfs interface and the new udisks2 interface
    //         we should convert the path from local volumn path to the dbus path which is
    //         used by our udisks2 wrapper classes.
    QString devicePath = this->getPath();
    devicePath.replace("dev", "org/freedesktop/UDisks2/block_devices");

    DFMBlockDevice *partition = DFMDiskManager::createBlockDevice(devicePath, nullptr);
    bool result = partition->canSetLabel();
    partition->deleteLater();

    return result;
}

QIcon UDiskDeviceInfo::fileIcon() const
{
    return fileIcon(128, 128);
}

QIcon UDiskDeviceInfo::fileIcon(int width, int height) const
{
#define QResIcon( freedesktop_icon_name, res_icon_name ) \
    QIcon::fromTheme( freedesktop_icon_name , QIcon(svgToHDPIPixmap(":/devices/images/device/" res_icon_name "-256px.svg", width, height)))

    if (getType() == "native") {
        return QResIcon("drive-harddisk", "drive-harddisk");
    } else if (getType() == "removable") {
        return QResIcon("drive-removable-media-usb", "drive-removable-media-usb");
    } else if (getType() == "network") {
        return QResIcon("drive-network", "drive-network");
    } else if (getType() == "phone") {
        return QResIcon("phone-android", "android-device");
    } else if (getType() == "iphone") {
        return QResIcon("phone-ios", "ios-device");
    } else if (getType() == "camera") {
        return QResIcon("camera-photo", "camera");
    } else if (getType() == "dvd") {
        return QResIcon("drive-optical", "media-dvd");
    } else {
        return QResIcon("drive-harddisk", "drive-harddisk");
    }
#undef QResIcon
}

bool UDiskDeviceInfo::isDir() const
{
    return true;
}

DUrl UDiskDeviceInfo::parentUrl() const
{
    return DUrl::fromComputerFile("/");
}

QVector<MenuAction> UDiskDeviceInfo::menuActionList(DAbstractFileInfo::MenuType type) const
{
    QVector<MenuAction> actionKeys;

    if (type == SpaceArea) {
        return actionKeys;
    }

    qDebug() << const_cast<UDiskDeviceInfo *>(this)->getMountPointUrl();

    actionKeys.reserve(6);

    actionKeys << MenuAction::OpenDisk
               << MenuAction::OpenDiskInNewWindow
               << MenuAction::OpenDiskInNewTab
               << MenuAction::Separator;

    if (canRename()) {
        actionKeys << MenuAction::Rename;
    }

    if (canEject()) {
        actionKeys << MenuAction::Eject;
    }

    if (canStop()) {
        actionKeys << MenuAction::SafelyRemoveDrive;
    }

    if (canUnmount()) {
        actionKeys << MenuAction::Unmount;
    } else {
        actionKeys << MenuAction::Mount;
    }

    if (getMediaType() == removable) {
        actionKeys << MenuAction::FormatDevice;
    }

    if (getId().startsWith("smb://")
            || getId().startsWith("ftp://")
            || getId().startsWith("sftp://")) {
        actionKeys << MenuAction::ForgetPassword;
    }
    actionKeys << MenuAction::Separator << MenuAction::Property;

    return actionKeys;
}

QSet<MenuAction> UDiskDeviceInfo::disableMenuActionList() const
{
    QSet<MenuAction> actionKeys = DAbstractFileInfo::disableMenuActionList();

    if (DFMGlobal::isRootUser()) {
        actionKeys << MenuAction::Unmount;
    }

    /*Disable unmount of native disk in x86 pro*/
    if (getMediaType() == native && DFMApplication::instance()->genericAttribute(DFMApplication::GA_DisableNonRemovableDeviceUnmount).toBool()) {
        actionKeys << MenuAction::Unmount;
    }

    if (!canUnmount()) {
        actionKeys << MenuAction::Property;
    }


    return actionKeys;
}

DUrl UDiskDeviceInfo::getUrlByNewFileName(const QString &fileName) const
{
    DUrl url = DUrl::fromDeviceId(getId());
    QUrlQuery query;
    query.addQueryItem("new_name", fileName);
    url.setQuery(query);

    return url;
}

bool UDiskDeviceInfo::canRedirectionFileUrl() const
{
    return !getMountPointUrl().isEmpty();
}

DUrl UDiskDeviceInfo::redirectedFileUrl() const
{
    return getMountPointUrl();
}

QVariantHash UDiskDeviceInfo::extensionPropertys() const
{
    QVariantHash attrMap;

    bool can_unmount = getMediaType() == native ? !DFMApplication::instance()->genericAttribute(DFMApplication::GA_DisableNonRemovableDeviceUnmount).toBool() : true;

    attrMap.insert("deviceId", getId());
    attrMap.insert("mediaType", static_cast<int>(getMediaType()));
    attrMap.insert("canMount", getDiskInfo().can_mount());
    attrMap.insert("canUnmount", canUnmount() && can_unmount);
    attrMap.insert("canEject", canEject() && can_unmount);
    attrMap.insert("canStop", canStop());
    attrMap.insert("isMounted", !getMountPointUrl().isEmpty());
    attrMap.insert("mountPointUrl", getMountPointUrl().toString());

    return attrMap;
}

bool UDiskDeviceInfo::exists() const
{
    if (fileUrl().isComputerFile()) {
        return true;
    }

    return true;
}

