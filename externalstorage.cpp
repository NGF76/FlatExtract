// ============================================================
// externalstorage.cpp
// اكتشاف وسائط التخزين الخارجية المثبتة على Linux
// ============================================================

#include "externalstorage.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSet>
#include <QStorageInfo>
#include <QStringList>

#include <algorithm>

namespace {

// مجلدات التحميل التي تستخدمها توزيعات Linux عادةً للوسائط القابلة للإزالة.
bool isExternalMountPath(const QString &path)
{
    static const QStringList prefixes = {
        QStringLiteral("/media/"),
        QStringLiteral("/run/media/"),
        QStringLiteral("/mnt/"),
        QStringLiteral("/Volumes/")
    };

    for (const QString &prefix : prefixes) {
        if (path.startsWith(prefix))
            return true;
    }
    return false;
}

// إرجاع اسم القرص الأب للقسم (sdb1 -> sdb، mmcblk0p1 -> mmcblk0).
QString parentBlockDevice(const QString &device)
{
    const QString name = QFileInfo(device).fileName();
    if (name.isEmpty())
        return QString();

    const QFileInfo blockInfo(QStringLiteral("/sys/class/block/%1").arg(name));
    const QString canonicalPath = blockInfo.canonicalFilePath();
    if (canonicalPath.isEmpty())
        return name;

    const QFileInfo canonicalInfo(canonicalPath);
    const QDir parentDir = canonicalInfo.dir();
    if (parentDir.dirName() == QStringLiteral("block"))
        return canonicalInfo.fileName(); // القرص نفسه وليس قسماً

    return parentDir.dirName();
}

// قراءة علم الإزالة من sysfs للتعرف على USB وبطاقات SD.
bool isRemovableDevice(const QString &device)
{
    if (!device.startsWith(QStringLiteral("/dev/")))
        return false;

    const QString disk = parentBlockDevice(device);
    if (disk.isEmpty())
        return false;

    QFile flag(QStringLiteral("/sys/class/block/%1/removable").arg(disk));
    if (!flag.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    return flag.readAll().trimmed() == "1";
}

} // namespace

QList<ExternalStorageVolume> externalStorageVolumes()
{
    QList<ExternalStorageVolume> volumes;
    QSet<QString> seenMounts;

    const QList<QStorageInfo> mounted = QStorageInfo::mountedVolumes();
    for (const QStorageInfo &storage : mounted) {
        if (!storage.isValid() || !storage.isReady())
            continue;

        const QString rootPath = QDir::cleanPath(storage.rootPath());
        if (rootPath.isEmpty() || rootPath == QStringLiteral("/"))
            continue;

        // استبعاد أقسام النظام والأنظمة الوهمية.
        if (rootPath.startsWith(QStringLiteral("/boot"))
                || rootPath.startsWith(QStringLiteral("/run/user"))
                || rootPath.startsWith(QStringLiteral("/snap"))) {
            continue;
        }

        const QString device = QString::fromLocal8Bit(storage.device());
        if (!isExternalMountPath(rootPath) && !isRemovableDevice(device))
            continue;

        if (seenMounts.contains(rootPath))
            continue;

        ExternalStorageVolume volume;
        volume.rootPath = rootPath;
        volume.device = device;
        volume.fileSystem = QString::fromLatin1(storage.fileSystemType());
        volume.bytesTotal = storage.bytesTotal();
        volume.bytesAvailable = storage.bytesAvailable();
        volume.readOnly = storage.isReadOnly();

        volume.displayName = storage.displayName().trimmed();
        if (volume.displayName.isEmpty() || volume.displayName == rootPath)
            volume.displayName = QFileInfo(rootPath).fileName();
        if (volume.displayName.isEmpty())
            volume.displayName = rootPath;

        volumes.append(volume);
        seenMounts.insert(rootPath);
    }

    std::sort(volumes.begin(), volumes.end(),
              [](const ExternalStorageVolume &a, const ExternalStorageVolume &b) {
                  const int nameCompare = QString::localeAwareCompare(a.displayName, b.displayName);
                  if (nameCompare != 0)
                      return nameCompare < 0;
                  return QString::localeAwareCompare(a.rootPath, b.rootPath) < 0;
              });

    return volumes;
}

QString nearestExistingDirectory(const QString &path)
{
    const QString trimmed = path.trimmed();
    if (trimmed.isEmpty())
        return QDir::homePath();

    QString candidate = QDir::cleanPath(trimmed);
    QFileInfo info(candidate);
    while (!info.exists() || !info.isDir()) {
        const QString parent = info.absolutePath();
        if (parent.isEmpty() || parent == candidate)
            break;
        candidate = parent;
        info.setFile(candidate);
    }

    return info.isDir() ? candidate : QDir::homePath();
}
