// ============================================================
// chdtool.cpp
// CHD image detection and conversion to ISO using chdman.
// ============================================================

#include "chdtool.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QProgressDialog>
#include <QStandardPaths>
#include <QWidget>
#include <QDebug>

namespace {

// Prefer a bundled chdman next to the application (AppImage layout),
// then fall back to chdman from PATH (provided by mame-tools).
QString resolveChdman()
{
    const QDir appDir(QCoreApplication::applicationDirPath());
    const QStringList candidates = {
        appDir.filePath(QStringLiteral("chdman")),
        appDir.filePath(QStringLiteral("../bin/chdman")),
        appDir.filePath(QStringLiteral("../lib/FlatExtract/chdman"))
    };

    for (const QString &candidate : candidates) {
        const QFileInfo info(candidate);
        if (info.exists() && info.isExecutable())
            return info.absoluteFilePath();
    }

    return QStringLiteral("chdman");
}

bool runChdman(const QString &executable, const QStringList &arguments,
               QWidget *dialogParent, QString &errorMessage)
{
    errorMessage.clear();

    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);

    bool failedToStart = false;
    bool canceled = false;
    QObject::connect(&process, &QProcess::errorOccurred, &process,
                     [&failedToStart](QProcess::ProcessError error) {
                         if (error == QProcess::FailedToStart)
                             failedToStart = true;
                     });

    QProgressDialog progress;
    if (dialogParent) {
        progress.setWindowTitle(QObject::tr("CHD conversion"));
        progress.setLabelText(QObject::tr("Converting the CHD image to ISO...\nThis can take a while for large games."));
        progress.setRange(0, 0);
        progress.setCancelButtonText(QObject::tr("Cancel"));
        progress.setWindowModality(Qt::WindowModal);
        progress.setMinimumDuration(400);
        progress.show();
    }

    process.start(executable, arguments);

    if (dialogParent) {
        QEventLoop loop;
        QObject::connect(&process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                         &loop, &QEventLoop::quit);
        QObject::connect(&process, &QProcess::errorOccurred, &loop, &QEventLoop::quit);
        QObject::connect(&progress, &QProgressDialog::canceled, &loop, [&]() {
            canceled = true;
            process.kill();
            loop.quit();
        });
        loop.exec();
    } else {
        if (!process.waitForStarted(10000)) {
            errorMessage = QObject::tr("Could not start chdman. "
                                       "Install the 'mame-tools' package or bundle chdman.");
            return false;
        }
        process.waitForFinished(-1);
    }

    progress.close();

    if (canceled) {
        errorMessage = QObject::tr("CHD conversion cancelled.");
        return false;
    }

    if (failedToStart) {
        errorMessage = QObject::tr("Could not start chdman. "
                                   "Install the 'mame-tools' package or bundle chdman.");
        return false;
    }

    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        QString output = QString::fromLocal8Bit(process.readAll()).trimmed();
        if (output.isEmpty()) {
            errorMessage = QObject::tr("chdman failed with exit code %1.")
                               .arg(process.exitCode());
        } else {
            errorMessage = output;
        }
        return false;
    }

    return true;
}

QString cachedIsoPath(const QFileInfo &chdInfo)
{
    const QString key = chdInfo.absoluteFilePath()
                        + QLatin1Char('|') + QString::number(chdInfo.size())
                        + QLatin1Char('|')
                        + QString::number(chdInfo.lastModified().toMSecsSinceEpoch());
    const QString hash = QString::fromLatin1(
        QCryptographicHash::hash(key.toUtf8(), QCryptographicHash::Sha1).toHex());
    return ChdTool::cacheDirectory() + QLatin1Char('/') + hash + QStringLiteral(".iso");
}

} // namespace

namespace ChdTool {

bool isChdFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return false;

    return file.read(8) == QByteArrayLiteral("MComprHD");
}

QString cacheDirectory()
{
    QString base = QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation);
    if (base.isEmpty())
        base = QDir::tempPath();

    const QString dir = base + QStringLiteral("/FlatExtract/chd");
    QDir().mkpath(dir);
    return dir;
}

void clearCache()
{
    QDir cache(cacheDirectory());
    cache.removeRecursively();
}

QString convertToIso(const QString &chdPath, QString &errorMessage,
                     QWidget *dialogParent)
{
    errorMessage.clear();

    const QFileInfo chdInfo(chdPath);
    if (!chdInfo.exists() || !chdInfo.isFile()) {
        errorMessage = QObject::tr("CHD file not found: %1").arg(chdPath);
        return QString();
    }

    if (!isChdFile(chdPath)) {
        errorMessage = QObject::tr("'%1' is not a valid CHD image.")
                           .arg(chdInfo.fileName());
        return QString();
    }

    const QString isoPath = cachedIsoPath(chdInfo);
    const QFileInfo cachedInfo(isoPath);
    if (cachedInfo.exists() && cachedInfo.size() > 0)
        return isoPath;

    const QString chdman = resolveChdman();

    // PSP UMD images are DVD-based, so try the DVD extractor first.
    QFile::remove(isoPath);
    QString dvdError;
    if (runChdman(chdman,
                  {QStringLiteral("extractdvd"),
                   QStringLiteral("-i"), chdPath,
                   QStringLiteral("-o"), isoPath,
                   QStringLiteral("-f")},
                  dialogParent, dvdError)) {
        if (QFileInfo(isoPath).size() > 0)
            return isoPath;
    }

    // Fall back to the CD extractor for CHDs created with "createcd".
    QFile::remove(isoPath);
    QString cdError;
    if (runChdman(chdman,
                  {QStringLiteral("extractcd"),
                   QStringLiteral("-i"), chdPath,
                   QStringLiteral("-o"), isoPath,
                   QStringLiteral("-f")},
                  dialogParent, cdError)) {
        if (QFileInfo(isoPath).size() > 0)
            return isoPath;
    }

    QFile::remove(isoPath);

    if (!cdError.isEmpty())
        errorMessage = cdError;
    else if (!dvdError.isEmpty())
        errorMessage = dvdError;
    else
        errorMessage = QObject::tr("Failed to convert the CHD image to ISO.");

    return QString();
}

} // namespace ChdTool
