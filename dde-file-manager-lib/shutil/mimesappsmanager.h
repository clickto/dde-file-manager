#ifndef MIMESAPPSMANAGER_H
#define MIMESAPPSMANAGER_H


#include <QObject>
#include <QSet>
#include <QMimeType>
#include <QMap>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QTimer>
#include "desktopfile.h"
#include <QIcon>

class MimeAppsWorker: public QObject
{
   Q_OBJECT

public:
    MimeAppsWorker(QObject *parent = 0);
    ~MimeAppsWorker();

    void initConnect();
public slots:
    void startWatch();
    void handleDirectoryChanged(const QString& filePath);
    void handleFileChanged(const QString& filePath);
    void updateCache();
    void writeData(const QString& path, const QByteArray& content);
    QByteArray readData(const QString& path);


private:
    QFileSystemWatcher* m_fileSystemWatcher = NULL;
    QTimer* m_updateCacheTimer;
};


class MimesAppsManager: public QObject
{
    Q_OBJECT

public:
    MimesAppsManager(QObject *parent = 0);
    ~MimesAppsManager();

    static QStringList DesktopFiles;
    static QMap<QString, QStringList> MimeApps;
    //specially cache for video, image, text and audio
    static QMap<QString, DesktopFile> VideoMimeApps;
    static QMap<QString, DesktopFile> ImageMimeApps;
    static QMap<QString, DesktopFile> TextMimeApps;
    static QMap<QString, DesktopFile> AudioMimeApps;
    static QMap<QString, DesktopFile> DesktopObjs;

    static QMimeType getMimeType(const QString& fileName);
    static QString getMimeTypeByFileName(const QString& fileName);
    static QString getDefaultAppByFileName(const QString& fileName);
    static QString getDefaultAppByMimeType(const QMimeType& mimeType);
    static QString getDefaultAppByMimeType(const QString& mimeType);
    static QString getDefaultAppDisplayNameByMimeType(const QMimeType& mimeType);
    static QString getDefaultAppDisplayNameByMimeType(const QString& mimeType);
    static void setDefautlAppForType(const QString& mimeType,
                                     const QString& targetAppName);

    static QStringList getRecommendedAppsByMimeType(const QMimeType& mimeType);


    static QStringList getApplicationsFolders();
    static QString getMimeAppsCacheFile();
    static QString getMimeInfoCacheFilePath();
    static QString getMimeInfoCacheFileRootPath();
    static QString getDesktopFilesCacheFile();
    static QString getDesktopIconsCacheFile();
    static QStringList getDesktopFiles();
    static QMap<QString, DesktopFile> getDesktopObjs();
    static void initMimeTypeApps();
    static bool lessByDateTime(const QFileInfo& f1,  const QFileInfo& f2);

signals:
    void requestUpdateCache();

private:
    MimeAppsWorker* m_mimeAppsWorker=NULL;

};

#endif // MIMESAPPSMANAGER_H
