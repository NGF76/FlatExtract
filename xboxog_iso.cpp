#include "xboxog_iso.h"

#include "iso_reader.h"

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStringList>
#include <QDebug>

namespace {

constexpr qint64 kSectorSize = 2048;
constexpr qint64 kVolumeDescriptorLba = 32;
const char kXdvdfsMagic[] = "MICROSOFT*XBOX*MEDIA";
constexpr quint32 kXbeMagic = 0x48454258;

bool hasXdvdfsAtZero(const QString &isoPath)
{
    QFile file(isoPath);
    if (!file.open(QIODevice::ReadOnly)) return false;
    if (!file.seek(kVolumeDescriptorLba * kSectorSize)) return false;
    return file.read(static_cast<qint64>(sizeof(kXdvdfsMagic)) - 1)
           == QByteArray(kXdvdfsMagic, static_cast<int>(sizeof(kXdvdfsMagic)) - 1);
}

bool isXbeFileName(const QString &path)
{
    return path.endsWith(QStringLiteral(".xbe"), Qt::CaseInsensitive);
}

QString safeRelativePath(const std::string &rawPath, bool &ok)
{
    QString path = QString::fromStdString(rawPath);
    path.replace(QLatin1Char('\\'), QLatin1Char('/'));

    const QStringList parts = path.split(QLatin1Char('/'), Qt::SkipEmptyParts);
    QStringList safeParts;

    ok = !parts.isEmpty();
    for (const QString &part : parts) {
        if (part.isEmpty() || part == QStringLiteral(".") || part == QStringLiteral("..")) {
            ok = false;
            break;
        }
        safeParts << part;
    }

    if (!ok) return QString();
    return safeParts.join(QLatin1Char('/'));
}

quint32 readLe32(const QByteArray &data, int offset)
{
    if (offset < 0 || offset + 4 > data.size()) return 0;
    const uchar *p = reinterpret_cast<const uchar *>(data.constData() + offset);
    return quint32(p[0])
           | (quint32(p[1]) << 8)
           | (quint32(p[2]) << 16)
           | (quint32(p[3]) << 24);
}

QString decodeUtf16Le(const QByteArray &data, int offset, int codeUnits)
{
    QString result;
    for (int i = 0; i < codeUnits; ++i) {
        const int pos = offset + i * 2;
        if (pos + 1 >= data.size()) break;

        const uchar *p = reinterpret_cast<const uchar *>(data.constData() + pos);
        const quint16 unit = quint16(p[0]) | (quint16(p[1]) << 8);
        if (unit == 0) break;

        result.append(QChar(unit));
    }
    return result;
}

}

bool isXboxOgIso(const QString &isoPath)
{
    if (!QFileInfo::exists(isoPath) || !hasXdvdfsAtZero(isoPath)) return false;

    IsoReader reader;
    if (!reader.Open(isoPath.toStdString())) return false;

    bool foundRootXbe = false;
    const auto entries = reader.ListAllFiles();
    for (const IsoFileEntry &entry : entries) {
        if (entry.isDirectory) continue;

        const QString path = QString::fromStdString(entry.path);
        if (path.contains(QLatin1Char('\\')) || !isXbeFileName(path)) continue;

        if (path.compare(QStringLiteral("default.xbe"), Qt::CaseInsensitive) == 0) {
            return true;
        }
        foundRootXbe = true;
    }

    return foundRootXbe;
}

bool readXboxOgXbeInfo(const QString &xbePath, XboxOgXbeInfo &info)
{
    info = XboxOgXbeInfo();

    QFile file(xbePath);
    if (!file.open(QIODevice::ReadOnly)) {
        info.error = QStringLiteral("Unable to open XBE file.");
        return false;
    }

    const QByteArray header = file.read(0x174);
    if (header.size() < 0x12C) {
        info.error = QStringLiteral("XBE header is truncated.");
        return false;
    }

    if (readLe32(header, 0) != kXbeMagic) {
        info.error = QStringLiteral("Invalid XBE magic.");
        return false;
    }

    info.baseAddress = readLe32(header, 0x104);
    info.sizeOfImage = readLe32(header, 0x10C);
    info.entryPoint = readLe32(header, 0x128);

    const quint32 certificateAddress = readLe32(header, 0x118);
    if (certificateAddress < info.baseAddress) {
        info.error = QStringLiteral("Invalid XBE certificate address.");
        return false;
    }

    const qint64 certificateOffset = static_cast<qint64>(certificateAddress - info.baseAddress);
    if (!file.seek(certificateOffset)) {
        info.error = QStringLiteral("XBE certificate is outside the file.");
        return false;
    }

    const QByteArray certificate = file.read(0x100);
    if (certificate.size() < 176) {
        info.error = QStringLiteral("XBE certificate is truncated.");
        return false;
    }

    info.titleId = readLe32(certificate, 8);
    info.titleName = decodeUtf16Le(certificate, 12, 40);
    info.region = readLe32(certificate, 160);
    info.version = readLe32(certificate, 172);
    info.valid = true;
    return true;
}

bool extractXboxOgIso(const QString &isoPath, const QString &outputPath,
                      QString &errorMessage,
                      XboxOgProgressCallback progressCallback)
{
    errorMessage.clear();

    if (!QFileInfo::exists(isoPath)) {
        errorMessage = QStringLiteral("The ISO file does not exist: ") + isoPath;
        return false;
    }

    if (!hasXdvdfsAtZero(isoPath)) {
        errorMessage = QStringLiteral("The selected ISO is not an original Xbox (XDVDFS) image.");
        return false;
    }

    if (outputPath.isEmpty()) {
        errorMessage = QStringLiteral("No output folder was selected.");
        return false;
    }

    IsoReader reader;
    if (!reader.Open(isoPath.toStdString())) {
        errorMessage = QStringLiteral("Failed to open the original Xbox ISO.");
        return false;
    }

    const auto entries = reader.ListAllFiles();
    if (entries.empty()) {
        errorMessage = QStringLiteral("No files were found in the original Xbox ISO.");
        return false;
    }

    if (!QDir().mkpath(outputPath)) {
        errorMessage = QStringLiteral("Failed to create the destination folder: ") + outputPath;
        return false;
    }

    const QString rootPath = QDir(outputPath).absolutePath();

    int totalFiles = 0;
    for (const IsoFileEntry &entry : entries) {
        if (!entry.isDirectory) ++totalFiles;
    }

    int currentFile = 0;
    int extractedFiles = 0;
    int failedFiles = 0;
    bool foundDefaultXbe = false;
    XboxOgXbeInfo defaultXbe;

    for (const IsoFileEntry &entry : entries) {
        bool validPath = false;
        const QString relativePath = safeRelativePath(entry.path, validPath);

        if (!validPath) {
            qWarning() << "Skipping unsafe Xbox OG path:" << QString::fromStdString(entry.path);
            if (!entry.isDirectory) {
                ++currentFile;
                if (progressCallback) {
                    progressCallback(currentFile, totalFiles, QString::fromStdString(entry.path));
                }
            }
            continue;
        }

        const QString outputFilePath = QDir(rootPath).filePath(relativePath);

        if (entry.isDirectory) {
            if (!QDir().mkpath(outputFilePath)) {
                qWarning() << "Failed to create Xbox OG directory:" << outputFilePath;
            }
            continue;
        }

        const QFileInfo outputInfo(outputFilePath);
        if (!QDir().mkpath(outputInfo.absolutePath())) {
            ++failedFiles;
            ++currentFile;
            qWarning() << "Failed to create parent directory for:" << outputFilePath;
            if (progressCallback) {
                progressCallback(currentFile, totalFiles, relativePath);
            }
            continue;
        }

        bool extracted = false;
        if (entry.fileSize == 0) {
            QFile emptyFile(outputFilePath);
            extracted = emptyFile.open(QIODevice::WriteOnly);
            if (extracted) emptyFile.close();
        } else {
            extracted = reader.ExtractBySectorToDisk(
                entry.startSector, entry.fileSize, outputFilePath.toStdString());
        }

        if (extracted) {
            ++extractedFiles;

            if (isXbeFileName(relativePath)) {
                XboxOgXbeInfo xbeInfo;
                if (readXboxOgXbeInfo(outputFilePath, xbeInfo)) {
                    qDebug() << "Xbox OG XBE:" << relativePath
                             << xbeInfo.titleName
                             << "titleId" << QString::number(xbeInfo.titleId, 16);

                    if (relativePath.compare(QStringLiteral("default.xbe"),
                                             Qt::CaseInsensitive) == 0) {
                        foundDefaultXbe = true;
                        defaultXbe = xbeInfo;
                    }
                } else {
                    qWarning() << "Unable to read XBE header:" << relativePath
                               << xbeInfo.error;
                }
            }
        } else {
            ++failedFiles;
            qWarning() << "Failed to extract Xbox OG file:" << relativePath;
        }

        ++currentFile;
        if (progressCallback) {
            progressCallback(currentFile, totalFiles, relativePath);
        }
    }

    if (extractedFiles == 0) {
        errorMessage = QStringLiteral("Failed to extract any file from the original Xbox ISO.");
        return false;
    }

    if (foundDefaultXbe) {
        qDebug() << "Xbox OG default.xbe:" << defaultXbe.titleName
                 << "region" << defaultXbe.region
                 << "version" << defaultXbe.version;
    } else {
        qWarning() << "No root default.xbe was found in the original Xbox ISO.";
    }

    if (failedFiles > 0) {
        qWarning() << failedFiles << "Xbox OG file(s) failed to extract.";
    }

    return true;
}
