// ============================================================
// gameiso.cpp
// PlayStation 2 (ISO 9660), PlayStation 3 (UDF), and
// GameCube (GCM/FST) disc image support.
// ============================================================

#include "gameiso.h"

#include <QByteArray>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLibrary>
#include <QSet>
#include <QStringList>
#include <QVector>
#include <QDebug>

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QTextCodec>
#endif

#include <archive.h>
#include <archive_entry.h>

#include <algorithm>
#include <cstring>
#include <functional>
#include <vector>

#include <sys/types.h>

namespace {

constexpr qint64 kSectorSize = 2048;
constexpr qint64 kMaxDescriptorSector = 32;
constexpr quint32 kGameCubeMagic = 0xC2339F3D;
constexpr int kMaxDirectoryDepth = 64;
constexpr int kMaxDirectoryEntries = 2000000;

const QString ERROR_FILE_NOT_FOUND = QStringLiteral("File not found: ");
const QString ERROR_OPEN_ISO = QStringLiteral("Failed to open ISO file: ");
const QString ERROR_CREATE_DIR = QStringLiteral("Failed to create the destination folder: ");
const QString ERROR_ISO9660_OPEN = QStringLiteral("Failed to open ISO 9660 image: ");
const QString ERROR_ISO9660_READ = QStringLiteral("Failed to read ISO 9660 image: ");
const QString ERROR_UDF_OPEN = QStringLiteral("Failed to open UDF image: ");
const QString ERROR_UDF_READ = QStringLiteral("Failed to read UDF image: ");
const QString ERROR_GAMECUBE_OPEN = QStringLiteral("Failed to open GameCube image: ");
const QString ERROR_GAMECUBE_READ = QStringLiteral("Failed to read GameCube image: ");
const QString ERROR_NO_FILES = QStringLiteral("No files were found in the disc image.");

// ============================================================
// Small helpers
// ============================================================

bool fileExists(const QString &filePath, QString &errorMessage)
{
    if (!QFileInfo(filePath).exists()) {
        errorMessage = ERROR_FILE_NOT_FOUND + filePath;
        return false;
    }
    return true;
}

bool createOutputDir(const QString &outputPath, QString &errorMessage)
{
    if (!QDir().mkpath(outputPath)) {
        errorMessage = ERROR_CREATE_DIR + outputPath;
        return false;
    }
    return true;
}

QByteArray readAt(QFile &file, qint64 offset, qint64 length)
{
    if (offset < 0 || length <= 0) return QByteArray();
    if (!file.seek(offset)) return QByteArray();
    return file.read(length);
}

quint32 readBe32(const QByteArray &data, int offset)
{
    if (offset < 0 || offset + 4 > data.size()) return 0;
    const uchar *p = reinterpret_cast<const uchar *>(data.constData() + offset);
    return (quint32(p[0]) << 24)
           | (quint32(p[1]) << 16)
           | (quint32(p[2]) << 8)
           | quint32(p[3]);
}

QString sanitizeRelativePath(const QString &rawPath, bool &ok)
{
    ok = false;

    QString path = rawPath;
    path.replace(QLatin1Char('\\'), QLatin1Char('/'));

    // ISO image paths may be absolute or prefixed with "./".
    while (path.startsWith(QLatin1String("./"))) {
        path.remove(0, 2);
    }
    while (path.startsWith(QLatin1Char('/'))) {
        path.remove(0, 1);
    }

    if (path.isEmpty()) return QString();

    const QStringList parts = path.split(QLatin1Char('/'), Qt::SkipEmptyParts);
    QStringList safeParts;

    for (const QString &part : parts) {
        if (part.isEmpty() || part == QLatin1String(".") || part == QLatin1String("..")) {
            return QString();
        }
        if (part.contains(QChar('\0'))) {
            return QString();
        }
        safeParts << part;
    }

    if (safeParts.isEmpty()) return QString();

    const QString result = safeParts.join(QLatin1Char('/'));
    if (result.size() > 4096) return QString();

    ok = true;
    return result;
}

QString archiveError(archive *reader)
{
    const char *message = archive_error_string(reader);
    return message ? QString::fromLocal8Bit(message) : QStringLiteral("Unknown archive error.");
}

bool isRootIsoPath(const QString &path)
{
    return path.isEmpty()
           || path == QLatin1String(".")
           || path == QLatin1String("./")
           || path == QLatin1String("/");
}

} // namespace

// ============================================================
// Format detection
// ============================================================

static bool hasXdvdfsSignature(QFile &file, qint64 fileSize, qint64 baseOffset)
{
    static const char magic[] = "MICROSOFT*XBOX*MEDIA";
    const qint64 offset = baseOffset + 32 * kSectorSize;
    if (offset < 0 || offset + static_cast<qint64>(sizeof(magic)) - 1 > fileSize) {
        return false;
    }

    const QByteArray data = readAt(file, offset, static_cast<qint64>(sizeof(magic)) - 1);
    return data == QByteArray(magic, static_cast<int>(sizeof(magic)) - 1);
}

static bool hasIso9660DescriptorAt(QFile &file, qint64 fileSize, qint64 sector)
{
    const qint64 offset = sector * kSectorSize;
    if (offset + 7 > fileSize) return false;

    // Volume descriptor: type byte followed by "CD001".
    const QByteArray data = readAt(file, offset, 7);
    if (data.size() < 7) return false;
    if (data.at(0) != 1 && data.at(0) != 2) return false;
    return data.mid(1, 5) == QByteArrayLiteral("CD001");
}

static bool hasUdfVolumeRecognitionSequence(QFile &file, qint64 fileSize)
{
    bool foundBea = false;
    bool foundNsr = false;

    for (qint64 sector = 16; sector < kMaxDescriptorSector; ++sector) {
        const qint64 offset = sector * kSectorSize;
        if (offset + 6 > fileSize) break;

        const QByteArray data = readAt(file, offset, 6);
        if (data.size() < 6) break;

        const QByteArray identifier = data.mid(1, 5);
        if (identifier == QByteArrayLiteral("BEA01")) {
            foundBea = true;
        } else if (identifier == QByteArrayLiteral("NSR02")
                   || identifier == QByteArrayLiteral("NSR03")) {
            foundNsr = true;
        }
    }

    return foundBea && foundNsr;
}

IsoImageFormat detectIsoImageFormat(const QString &isoPath)
{
    QFile file(isoPath);
    if (!file.open(QIODevice::ReadOnly)) {
        return IsoImageFormat::Unknown;
    }

    const qint64 fileSize = file.size();
    if (fileSize < 0x20) {
        return IsoImageFormat::Unknown;
    }

    // XDVDFS can start at the beginning of the image or after an offset.
    static const qint64 xdvdfsBases[] = {
        0,
        0x0FD90000LL,
        0x02080000LL
    };

    for (qint64 base : xdvdfsBases) {
        if (hasXdvdfsSignature(file, fileSize, base)) {
            return IsoImageFormat::XboxXdvdfs;
        }
    }

    // GameCube GCM/FST boot magic.
    if (fileSize >= 0x440) {
        const QByteArray header = readAt(file, 0x1C, 4);
        if (header.size() == 4 && readBe32(header, 0) == kGameCubeMagic) {
            return IsoImageFormat::GameCube;
        }
    }

    // Prefer UDF when both UDF and ISO 9660 descriptors exist (UDF bridge).
    if (hasUdfVolumeRecognitionSequence(file, fileSize)) {
        return IsoImageFormat::Udf;
    }

    for (qint64 sector = 16; sector < kMaxDescriptorSector; ++sector) {
        if (hasIso9660DescriptorAt(file, fileSize, sector)) {
            return IsoImageFormat::Iso9660;
        }
    }

    return IsoImageFormat::Unknown;
}

QString isoImageFormatName(IsoImageFormat format)
{
    switch (format) {
    case IsoImageFormat::XboxXdvdfs:
        return QStringLiteral("Xbox XDVDFS");
    case IsoImageFormat::GameCube:
        return QStringLiteral("GameCube");
    case IsoImageFormat::Iso9660:
        return QStringLiteral("ISO 9660");
    case IsoImageFormat::Udf:
        return QStringLiteral("UDF");
    case IsoImageFormat::Unknown:
        break;
    }

    return QStringLiteral("Unknown");
}

bool hasIso9660FileSystem(const QString &isoPath)
{
    QFile file(isoPath);
    if (!file.open(QIODevice::ReadOnly)) return false;

    const qint64 fileSize = file.size();
    for (qint64 sector = 16; sector < kMaxDescriptorSector; ++sector) {
        if (hasIso9660DescriptorAt(file, fileSize, sector)) {
            return true;
        }
    }
    return false;
}

// ============================================================
// ISO 9660 (PlayStation 2)
// ============================================================

archive *openIso9660Archive(const QString &isoPath, QString &errorMessage)
{
    archive *reader = archive_read_new();
    if (!reader) {
        errorMessage = ERROR_ISO9660_OPEN + QStringLiteral("failed to allocate archive reader.");
        return nullptr;
    }

    archive_read_support_filter_all(reader);
    archive_read_support_format_iso9660(reader);

    const QByteArray nativePath = QFile::encodeName(isoPath);
    if (archive_read_open_filename(reader, nativePath.constData(), 65536) != ARCHIVE_OK) {
        errorMessage = ERROR_ISO9660_OPEN + archiveError(reader);
        archive_read_free(reader);
        return nullptr;
    }

    return reader;
}

int countIso9660Files(const QString &isoPath, QString &errorMessage)
{
    archive *reader = openIso9660Archive(isoPath, errorMessage);
    if (!reader) return -1;

    int count = 0;
    archive_entry *entry = nullptr;
    int result = ARCHIVE_OK;

    while ((result = archive_read_next_header(reader, &entry)) == ARCHIVE_OK
           || result == ARCHIVE_WARN) {
        if (archive_entry_filetype(entry) == AE_IFREG) {
            ++count;
        }
        archive_read_data_skip(reader);
    }

    if (result != ARCHIVE_EOF) {
        errorMessage = ERROR_ISO9660_READ + archiveError(reader);
        archive_read_free(reader);
        return -1;
    }

    archive_read_free(reader);
    return count;
}

QStringList listIso9660Contents(const QString &isoPath, QString &errorMessage)
{
    errorMessage.clear();
    QStringList files;

    if (!fileExists(isoPath, errorMessage)) return files;

    archive *reader = openIso9660Archive(isoPath, errorMessage);
    if (!reader) return files;

    QSet<QString> seen;
    archive_entry *entry = nullptr;
    int result = ARCHIVE_OK;

    while ((result = archive_read_next_header(reader, &entry)) == ARCHIVE_OK
           || result == ARCHIVE_WARN) {
        const char *pathname = archive_entry_pathname(entry);
        if (!pathname) {
            archive_read_data_skip(reader);
            continue;
        }

        const mode_t entryType = archive_entry_filetype(entry);
        if (entryType != AE_IFREG && entryType != AE_IFDIR) {
            archive_read_data_skip(reader);
            continue;
        }

        const QString rawPath = QString::fromUtf8(pathname);
        if (isRootIsoPath(rawPath)) {
            archive_read_data_skip(reader);
            continue;
        }

        bool validPath = false;
        const QString relativePath = sanitizeRelativePath(rawPath, validPath);
        if (validPath && !seen.contains(relativePath)) {
            seen.insert(relativePath);
            files << relativePath;
        } else if (!validPath) {
            qWarning() << "Skipping unsafe ISO 9660 path:" << rawPath;
        }

        archive_read_data_skip(reader);
    }

    if (result != ARCHIVE_EOF) {
        errorMessage = ERROR_ISO9660_READ + archiveError(reader);
        files.clear();
    }

    archive_read_free(reader);
    files.sort(Qt::CaseInsensitive);
    return files;
}

bool extractIso9660Image(const QString &isoPath, const QString &outputPath,
                         QString &errorMessage,
                         ExtractProgressCallback progressCallback)
{
    errorMessage.clear();

    if (!fileExists(isoPath, errorMessage)) return false;

    if (outputPath.isEmpty()) {
        errorMessage = QStringLiteral("No output folder was selected.");
        return false;
    }

    if (!createOutputDir(outputPath, errorMessage)) return false;

    const QString rootPath = QDir(outputPath).absolutePath();

    const int totalFiles = countIso9660Files(isoPath, errorMessage);
    if (totalFiles < 0) return false;

    archive *reader = openIso9660Archive(isoPath, errorMessage);
    if (!reader) return false;

    int currentFile = 0;
    int extractedFiles = 0;
    int failedFiles = 0;
    archive_entry *entry = nullptr;
    int result = ARCHIVE_OK;
    std::vector<char> buffer(1024 * 1024);

    while ((result = archive_read_next_header(reader, &entry)) == ARCHIVE_OK
           || result == ARCHIVE_WARN) {
        const char *pathname = archive_entry_pathname(entry);
        if (!pathname) {
            archive_read_data_skip(reader);
            continue;
        }

        const mode_t entryType = archive_entry_filetype(entry);
        const QString rawPath = QString::fromUtf8(pathname);
        if (isRootIsoPath(rawPath)) {
            archive_read_data_skip(reader);
            continue;
        }

        bool validPath = false;
        const QString relativePath = sanitizeRelativePath(rawPath, validPath);

        if (entryType == AE_IFDIR) {
            if (validPath) {
                QDir().mkpath(QDir(rootPath).filePath(relativePath));
            }
            archive_read_data_skip(reader);
            continue;
        }

        if (entryType != AE_IFREG) {
            archive_read_data_skip(reader);
            continue;
        }

        if (!validPath) {
            qWarning() << "Skipping unsafe ISO 9660 path:" << rawPath;
            ++currentFile;
            if (progressCallback) {
                progressCallback(currentFile, totalFiles, rawPath);
            }
            archive_read_data_skip(reader);
            continue;
        }

        const QString outputFilePath = QDir(rootPath).filePath(relativePath);
        const QFileInfo outputInfo(outputFilePath);

        if (!QDir().mkpath(outputInfo.absolutePath())) {
            ++failedFiles;
            ++currentFile;
            qWarning() << "Failed to create parent directory for:" << outputFilePath;
            if (progressCallback) {
                progressCallback(currentFile, totalFiles, relativePath);
            }
            archive_read_data_skip(reader);
            continue;
        }

        QFile outputFile(outputFilePath);
        bool extracted = outputFile.open(QIODevice::WriteOnly | QIODevice::Truncate);
        qint64 written = 0;

        if (extracted) {
            while (true) {
                const la_ssize_t bytesRead =
                    archive_read_data(reader, buffer.data(), buffer.size());

                if (bytesRead < 0) {
                    qWarning() << "ISO 9660 read error for:" << relativePath
                               << archiveError(reader);
                    extracted = false;
                    break;
                }
                if (bytesRead == 0) break;

                if (outputFile.write(buffer.data(), bytesRead) != bytesRead) {
                    qWarning() << "Failed to write extracted file:" << outputFilePath;
                    extracted = false;
                    break;
                }
                written += bytesRead;
            }

            const la_int64_t expectedSize = archive_entry_size(entry);
            if (extracted && archive_entry_size_is_set(entry)
                && expectedSize >= 0 && written != expectedSize) {
                qWarning() << "Short read for ISO 9660 file:" << relativePath
                           << "expected" << expectedSize << "got" << written;
                extracted = false;
            }

            outputFile.close();
        }

        if (!extracted) {
            ++failedFiles;
            outputFile.remove();
        } else {
            ++extractedFiles;
        }

        ++currentFile;
        if (progressCallback) {
            progressCallback(currentFile, totalFiles, relativePath);
        }
    }

    if (result != ARCHIVE_EOF) {
        errorMessage = ERROR_ISO9660_READ + archiveError(reader);
        archive_read_free(reader);
        return false;
    }

    archive_read_free(reader);

    if (extractedFiles == 0) {
        errorMessage = QStringLiteral("Failed to extract any file from the ISO 9660 image.");
        return false;
    }

    if (failedFiles > 0) {
        qWarning() << failedFiles << "ISO 9660 file(s) failed to extract.";
    }

    return true;
}

// ============================================================
// UDF (PlayStation 3)
// ============================================================

namespace {

struct UdfDirent
{
    unsigned int d_type;
    const char *d_name;
};

struct UdfEntry
{
    QString path;
    bool isDirectory = false;
};

class UdfApi
{
public:
    ~UdfApi()
    {
        if (lib.isLoaded()) {
            lib.unload();
        }
    }

    bool load(QString &errorMessage)
    {
        const QString appDir = QCoreApplication::applicationDirPath();
        QStringList candidates;

        if (!appDir.isEmpty()) {
            candidates << appDir + QStringLiteral("/libudfread.so.0")
                       << appDir + QStringLiteral("/lib/libudfread.so.0")
                       << appDir + QStringLiteral("/../lib/libudfread.so.0");
        }
        candidates << QStringLiteral("libudfread.so.0")
                   << QStringLiteral("libudfread.so")
                   << QStringLiteral("udfread");

        for (const QString &candidate : candidates) {
            lib.setFileName(candidate);
            if (lib.load()) {
                break;
            }
        }

        if (!lib.isLoaded()) {
            errorMessage = QStringLiteral(
                "libudfread is not available. Install the libudfread0 package "
                "to extract PlayStation 3 UDF images. (%1)")
                .arg(lib.errorString());
            return false;
        }

        bool resolved = true;
        resolved &= resolve(init, "udfread_init");
        resolved &= resolve(open, "udfread_open");
        resolved &= resolve(close, "udfread_close");
        resolved &= resolve(opendir, "udfread_opendir");
        resolved &= resolve(readdir, "udfread_readdir");
        resolved &= resolve(closedir, "udfread_closedir");
        resolved &= resolve(fileOpen, "udfread_file_open");
        resolved &= resolve(fileClose, "udfread_file_close");
        resolved &= resolve(fileSize, "udfread_file_size");
        resolved &= resolve(fileRead, "udfread_file_read");

        if (!resolved) {
            errorMessage = QStringLiteral(
                "The installed libudfread library is missing required functions.");
            lib.unload();
            return false;
        }

        return true;
    }

    template <typename T>
    bool resolve(T &target, const char *name)
    {
        target = reinterpret_cast<T>(lib.resolve(name));
        return target != nullptr;
    }

    QLibrary lib;

    void *(*init)() = nullptr;
    int (*open)(void *, const char *) = nullptr;
    void (*close)(void *) = nullptr;
    void *(*opendir)(void *, const char *) = nullptr;
    UdfDirent *(*readdir)(void *, UdfDirent *) = nullptr;
    void (*closedir)(void *) = nullptr;
    void *(*fileOpen)(void *, const char *) = nullptr;
    void (*fileClose)(void *) = nullptr;
    int64_t (*fileSize)(void *) = nullptr;
    ssize_t (*fileRead)(void *, void *, size_t) = nullptr;
};

class UdfVolumeGuard
{
public:
    UdfVolumeGuard(UdfApi &api, void *volume)
        : m_api(api)
        , m_volume(volume)
    {
    }

    ~UdfVolumeGuard()
    {
        if (m_volume) m_api.close(m_volume);
    }

    UdfVolumeGuard(const UdfVolumeGuard &) = delete;
    UdfVolumeGuard &operator=(const UdfVolumeGuard &) = delete;

private:
    UdfApi &m_api;
    void *m_volume;
};

void *openUdfVolume(UdfApi &api, const QString &isoPath, QString &errorMessage)
{
    QString loadError;
    if (!api.load(loadError)) {
        errorMessage = loadError;
        return nullptr;
    }

    void *volume = api.init();
    if (!volume) {
        errorMessage = QStringLiteral("Failed to initialize the UDF reader.");
        return nullptr;
    }

    const QByteArray nativePath = QFile::encodeName(isoPath);
    if (api.open(volume, nativePath.constData()) < 0) {
        errorMessage = ERROR_UDF_OPEN + isoPath;
        api.close(volume);
        return nullptr;
    }

    return volume;
}

bool walkUdfDirectory(UdfApi &api, void *volume, const QString &relativeDirectory,
                      QVector<UdfEntry> &entries, QSet<QString> &visitedDirectories,
                      QString &errorMessage, int depth, int &entryCount)
{
    if (depth > kMaxDirectoryDepth) {
        errorMessage = QStringLiteral("UDF directory nesting is too deep.");
        return false;
    }

    if (visitedDirectories.contains(relativeDirectory)) {
        return true;
    }
    visitedDirectories.insert(relativeDirectory);

    const QString udfPath = relativeDirectory.isEmpty()
        ? QStringLiteral("/")
        : QLatin1Char('/') + relativeDirectory;

    const QByteArray nativePath = udfPath.toUtf8();
    void *directory = api.opendir(volume, nativePath.constData());
    if (!directory) {
        errorMessage = QStringLiteral("Failed to open UDF directory: ") + udfPath;
        return false;
    }

    bool ok = true;
    UdfDirent dirent{};
    while (api.readdir(directory, &dirent)) {
        if (!dirent.d_name) continue;

        const QString name = QString::fromUtf8(dirent.d_name);
        if (name.isEmpty() || name == QLatin1String(".") || name == QLatin1String("..")) {
            continue;
        }

        if (name.contains(QLatin1Char('/')) || name.contains(QLatin1Char('\\'))) {
            qWarning() << "Skipping unsafe UDF path component:" << name;
            continue;
        }

        if (++entryCount > kMaxDirectoryEntries) {
            errorMessage = QStringLiteral("UDF image contains too many entries.");
            ok = false;
            break;
        }

        const QString childPath = relativeDirectory.isEmpty()
            ? name
            : relativeDirectory + QLatin1Char('/') + name;

        if (dirent.d_type == 1) {
            UdfEntry entry;
            entry.path = childPath;
            entry.isDirectory = true;
            entries << entry;

            if (!walkUdfDirectory(api, volume, childPath, entries,
                                  visitedDirectories, errorMessage, depth + 1,
                                  entryCount)) {
                ok = false;
                break;
            }
        } else if (dirent.d_type == 2) {
            UdfEntry entry;
            entry.path = childPath;
            entry.isDirectory = false;
            entries << entry;
        } else {
            qWarning() << "Skipping UDF entry with unknown type:" << childPath;
        }
    }

    api.closedir(directory);
    return ok;
}

bool collectUdfEntries(const QString &isoPath, QVector<UdfEntry> &entries,
                       QString &errorMessage)
{
    errorMessage.clear();
    entries.clear();

    if (!fileExists(isoPath, errorMessage)) return false;

    UdfApi api;
    void *volume = openUdfVolume(api, isoPath, errorMessage);
    if (!volume) return false;
    UdfVolumeGuard volumeGuard(api, volume);

    QSet<QString> visitedDirectories;
    int entryCount = 0;
    if (!walkUdfDirectory(api, volume, QString(), entries, visitedDirectories,
                          errorMessage, 0, entryCount)) {
        return false;
    }

    return true;
}

} // namespace

QStringList listUdfContents(const QString &isoPath, QString &errorMessage)
{
    QVector<UdfEntry> entries;
    if (!collectUdfEntries(isoPath, entries, errorMessage)) {
        return QStringList();
    }

    QStringList files;
    files.reserve(entries.size());
    for (const UdfEntry &entry : entries) {
        files << entry.path;
    }
    files.sort(Qt::CaseInsensitive);
    return files;
}

bool extractUdfImage(const QString &isoPath, const QString &outputPath,
                     QString &errorMessage,
                     ExtractProgressCallback progressCallback)
{
    errorMessage.clear();

    if (outputPath.isEmpty()) {
        errorMessage = QStringLiteral("No output folder was selected.");
        return false;
    }

    QVector<UdfEntry> entries;
    if (!collectUdfEntries(isoPath, entries, errorMessage)) {
        return false;
    }

    int totalFiles = 0;
    for (const UdfEntry &entry : entries) {
        if (!entry.isDirectory) ++totalFiles;
    }

    if (totalFiles == 0) {
        errorMessage = ERROR_NO_FILES;
        return false;
    }

    if (!createOutputDir(outputPath, errorMessage)) return false;

    UdfApi api;
    void *volume = openUdfVolume(api, isoPath, errorMessage);
    if (!volume) return false;
    UdfVolumeGuard volumeGuard(api, volume);

    const QString rootPath = QDir(outputPath).absolutePath();
    std::vector<char> buffer(1024 * 1024);
    int currentFile = 0;
    int extractedFiles = 0;
    int failedFiles = 0;

    for (const UdfEntry &entry : entries) {
        bool validPath = false;
        const QString relativePath = sanitizeRelativePath(entry.path, validPath);

        if (!validPath) {
            qWarning() << "Skipping unsafe UDF path:" << entry.path;
            if (!entry.isDirectory) {
                ++currentFile;
                if (progressCallback) {
                    progressCallback(currentFile, totalFiles, entry.path);
                }
            }
            continue;
        }

        const QString outputFilePath = QDir(rootPath).filePath(relativePath);

        if (entry.isDirectory) {
            QDir().mkpath(outputFilePath);
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

        const QString udfPath = QLatin1Char('/') + relativePath;
        const QByteArray nativeUdfPath = udfPath.toUtf8();
        void *udfFile = api.fileOpen(volume, nativeUdfPath.constData());
        bool extracted = udfFile != nullptr;

        if (extracted) {
            const int64_t fileSize = api.fileSize(udfFile);
            if (fileSize < 0) {
                qWarning() << "Failed to get UDF file size:" << relativePath;
                extracted = false;
            }

            QFile outputFile(outputFilePath);
            if (extracted && !outputFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                qWarning() << "Failed to create output file:" << outputFilePath;
                extracted = false;
            }

            qint64 written = 0;
            if (extracted) {
                while (true) {
                    const ssize_t bytesRead =
                        api.fileRead(udfFile, buffer.data(), buffer.size());

                    if (bytesRead < 0) {
                        qWarning() << "UDF read error for:" << relativePath;
                        extracted = false;
                        break;
                    }
                    if (bytesRead == 0) break;

                    if (outputFile.write(buffer.data(), bytesRead) != bytesRead) {
                        qWarning() << "Failed to write extracted file:" << outputFilePath;
                        extracted = false;
                        break;
                    }
                    written += bytesRead;
                }

                if (extracted && fileSize >= 0 && written != fileSize) {
                    qWarning() << "Short read for UDF file:" << relativePath
                               << "expected" << fileSize << "got" << written;
                    extracted = false;
                }

                outputFile.close();
                if (!extracted) outputFile.remove();
            }

            api.fileClose(udfFile);
        } else {
            qWarning() << "Failed to open UDF file:" << relativePath;
        }

        if (extracted) {
            ++extractedFiles;
        } else {
            ++failedFiles;
        }

        ++currentFile;
        if (progressCallback) {
            progressCallback(currentFile, totalFiles, relativePath);
        }
    }

    if (extractedFiles == 0) {
        errorMessage = QStringLiteral("Failed to extract any file from the UDF image.");
        return false;
    }

    if (failedFiles > 0) {
        qWarning() << failedFiles << "UDF file(s) failed to extract.";
    }

    return true;
}

// ============================================================
// GameCube GCM/FST
// ============================================================

namespace {

struct GameCubeEntry
{
    QString path;
    bool isDirectory = false;
    quint64 offset = 0;
    quint64 size = 0;
};

QString decodeGameCubeName(const QByteArray &rawName)
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    if (QTextCodec *codec = QTextCodec::codecForName("Shift-JIS")) {
        return codec->toUnicode(rawName);
    }
#endif
    return QString::fromLatin1(rawName);
}

bool isValidGameCubeName(const QString &name)
{
    return !name.isEmpty()
           && name != QLatin1String(".")
           && name != QLatin1String("..")
           && !name.contains(QLatin1Char('/'))
           && !name.contains(QLatin1Char('\\'))
           && !name.contains(QChar('\0'));
}

bool parseGameCubeEntries(const QString &isoPath, QVector<GameCubeEntry> &entries,
                          QString &errorMessage)
{
    errorMessage.clear();
    entries.clear();

    QFile file(isoPath);
    if (!file.open(QIODevice::ReadOnly)) {
        errorMessage = ERROR_GAMECUBE_OPEN + isoPath;
        return false;
    }

    const qint64 fileSize = file.size();
    if (fileSize < 0x440) {
        errorMessage = QStringLiteral("The GameCube image is too small.");
        return false;
    }

    const QByteArray header = readAt(file, 0, 0x440);
    if (header.size() < 0x440 || readBe32(header, 0x1C) != kGameCubeMagic) {
        errorMessage = QStringLiteral("The selected image is not a GameCube GCM/ISO image.");
        return false;
    }

    const quint32 fstOffset = readBe32(header, 0x424);
    const quint32 fstSize = readBe32(header, 0x428);

    if (fstSize < 12 || fstSize > 64 * 1024 * 1024) {
        errorMessage = QStringLiteral("The GameCube FST has an invalid size.");
        return false;
    }

    if (qint64(fstOffset) + qint64(fstSize) > fileSize) {
        errorMessage = QStringLiteral("The GameCube FST is outside the image.");
        return false;
    }

    const QByteArray fst = readAt(file, fstOffset, fstSize);
    if (fst.size() != static_cast<int>(fstSize)) {
        errorMessage = ERROR_GAMECUBE_READ + QStringLiteral("failed to read the FST.");
        return false;
    }

    if (quint8(fst.at(0)) != 1) {
        errorMessage = QStringLiteral("The GameCube FST root is not a directory.");
        return false;
    }

    const quint32 totalEntries = readBe32(fst, 8);
    if (totalEntries == 0
        || totalEntries > static_cast<quint32>(fstSize / 12)
        || qint64(totalEntries) * 12 > fstSize) {
        errorMessage = QStringLiteral("The GameCube FST has an invalid entry count.");
        return false;
    }

    const int stringTableOffset = static_cast<int>(totalEntries) * 12;
    QSet<QString> seenPaths;

    auto entryName = [&](quint32 index) -> QString {
        if (qint64(index) * 12 + 12 > fst.size()) return QString();

        const quint32 typeAndNameOffset = readBe32(fst, static_cast<int>(index) * 12);
        const int nameOffset = static_cast<int>(typeAndNameOffset & 0x00FFFFFF);
        const int nameStart = stringTableOffset + nameOffset;
        if (nameStart < 0 || nameStart >= fst.size()) return QString();

        int nameEnd = nameStart;
        while (nameEnd < fst.size() && fst.at(nameEnd) != '\0') {
            ++nameEnd;
        }
        if (nameEnd >= fst.size()) return QString();

        return decodeGameCubeName(fst.mid(nameStart, nameEnd - nameStart));
    };

    std::function<bool(quint32, const QString &)> walkDirectory;
    walkDirectory = [&](quint32 directoryIndex, const QString &parentPath) -> bool {
        if (directoryIndex >= totalEntries) {
            errorMessage = QStringLiteral("The GameCube FST contains an invalid directory index.");
            return false;
        }

        const quint32 directoryEnd = readBe32(
            fst, static_cast<int>(directoryIndex) * 12 + 8);

        if (directoryEnd <= directoryIndex || directoryEnd > totalEntries) {
            errorMessage = QStringLiteral("The GameCube FST contains an invalid directory range.");
            return false;
        }

        for (quint32 index = directoryIndex + 1; index < directoryEnd; ) {
            if (index >= totalEntries) {
                errorMessage = QStringLiteral("The GameCube FST ended unexpectedly.");
                return false;
            }

            const int entryOffset = static_cast<int>(index) * 12;
            const quint8 type = quint8(fst.at(entryOffset));
            const QString name = entryName(index);

            if (!isValidGameCubeName(name)) {
                errorMessage = QStringLiteral("The GameCube FST contains an invalid file name.");
                return false;
            }

            const QString childPath = parentPath.isEmpty()
                ? name
                : parentPath + QLatin1Char('/') + name;

            if (type == 1) {
                const quint32 childEnd = readBe32(fst, entryOffset + 8);
                if (childEnd <= index || childEnd > directoryEnd) {
                    errorMessage = QStringLiteral("The GameCube FST contains an invalid subdirectory.");
                    return false;
                }

                GameCubeEntry entry;
                entry.path = childPath;
                entry.isDirectory = true;
                entries << entry;

                if (!walkDirectory(index, childPath)) {
                    return false;
                }
                index = childEnd;
            } else if (type == 0) {
                const quint64 fileOffset = readBe32(fst, entryOffset + 4);
                const quint64 dataSize = readBe32(fst, entryOffset + 8);

                if (fileOffset > quint64(fileSize)
                    || dataSize > quint64(fileSize) - fileOffset) {
                    errorMessage = QStringLiteral("The GameCube FST points outside the image: ")
                                   + childPath;
                    return false;
                }

                GameCubeEntry entry;
                entry.path = childPath;
                entry.isDirectory = false;
                entry.offset = fileOffset;
                entry.size = dataSize;
                entries << entry;
                ++index;
            } else {
                errorMessage = QStringLiteral("The GameCube FST contains an unknown entry type.");
                return false;
            }
        }

        return true;
    };

    if (!walkDirectory(0, QString())) {
        return false;
    }

    // Keep the FST order, but reject duplicated paths defensively.
    for (const GameCubeEntry &entry : entries) {
        if (seenPaths.contains(entry.path)) {
            errorMessage = QStringLiteral("The GameCube FST contains duplicate paths.");
            entries.clear();
            return false;
        }
        seenPaths.insert(entry.path);
    }

    if (entries.isEmpty()) {
        errorMessage = ERROR_NO_FILES;
        return false;
    }

    return true;
}

} // namespace

QStringList listGameCubeContents(const QString &isoPath, QString &errorMessage)
{
    QVector<GameCubeEntry> entries;
    if (!parseGameCubeEntries(isoPath, entries, errorMessage)) {
        return QStringList();
    }

    QStringList files;
    files.reserve(entries.size());
    for (const GameCubeEntry &entry : entries) {
        files << entry.path;
    }
    return files;
}

bool extractGameCubeImage(const QString &isoPath, const QString &outputPath,
                          QString &errorMessage,
                          ExtractProgressCallback progressCallback)
{
    errorMessage.clear();

    if (outputPath.isEmpty()) {
        errorMessage = QStringLiteral("No output folder was selected.");
        return false;
    }

    QVector<GameCubeEntry> entries;
    if (!parseGameCubeEntries(isoPath, entries, errorMessage)) {
        return false;
    }

    int totalFiles = 0;
    for (const GameCubeEntry &entry : entries) {
        if (!entry.isDirectory) ++totalFiles;
    }

    if (!createOutputDir(outputPath, errorMessage)) return false;

    QFile isoFile(isoPath);
    if (!isoFile.open(QIODevice::ReadOnly)) {
        errorMessage = ERROR_GAMECUBE_OPEN + isoPath;
        return false;
    }

    const QString rootPath = QDir(outputPath).absolutePath();
    std::vector<char> buffer(1024 * 1024);
    int currentFile = 0;
    int extractedFiles = 0;
    int failedFiles = 0;

    for (const GameCubeEntry &entry : entries) {
        bool validPath = false;
        const QString relativePath = sanitizeRelativePath(entry.path, validPath);

        if (!validPath) {
            qWarning() << "Skipping unsafe GameCube path:" << entry.path;
            if (!entry.isDirectory) {
                ++currentFile;
                if (progressCallback) {
                    progressCallback(currentFile, totalFiles, entry.path);
                }
            }
            continue;
        }

        const QString outputFilePath = QDir(rootPath).filePath(relativePath);

        if (entry.isDirectory) {
            QDir().mkpath(outputFilePath);
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

        QFile outputFile(outputFilePath);
        bool extracted = outputFile.open(QIODevice::WriteOnly | QIODevice::Truncate);
        quint64 remaining = entry.size;

        if (extracted && remaining > 0) {
            if (!isoFile.seek(static_cast<qint64>(entry.offset))) {
                qWarning() << "Failed to seek in the GameCube image for:" << relativePath;
                extracted = false;
            } else {
                while (remaining > 0) {
                    const qint64 chunk = static_cast<qint64>(
                        std::min<quint64>(remaining, buffer.size()));
                    const qint64 bytesRead = isoFile.read(buffer.data(), chunk);

                    if (bytesRead <= 0) {
                        qWarning() << "Unexpected end of GameCube image for:" << relativePath;
                        extracted = false;
                        break;
                    }

                    if (outputFile.write(buffer.data(), bytesRead) != bytesRead) {
                        qWarning() << "Failed to write extracted file:" << outputFilePath;
                        extracted = false;
                        break;
                    }

                    remaining -= static_cast<quint64>(bytesRead);
                }
            }
        }

        if (extracted && remaining != 0) {
            extracted = false;
        }

        outputFile.close();

        if (!extracted) {
            ++failedFiles;
            outputFile.remove();
        } else {
            ++extractedFiles;
        }

        ++currentFile;
        if (progressCallback) {
            progressCallback(currentFile, totalFiles, relativePath);
        }
    }

    if (extractedFiles == 0) {
        errorMessage = QStringLiteral("Failed to extract any file from the GameCube image.");
        return false;
    }

    if (failedFiles > 0) {
        qWarning() << failedFiles << "GameCube file(s) failed to extract.";
    }

    return true;
}
