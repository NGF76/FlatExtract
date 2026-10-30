#include "tarextract.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDebug>

#include <archive.h>
#include <archive_entry.h>

namespace {

const QString ERROR_FILE_NOT_FOUND = "File not found: ";
const QString ERROR_UNSUPPORTED = "Unsupported archive format.";
const QString ERROR_OPEN = "Failed to open archive: ";
const QString ERROR_READ = "Failed to read archive: ";
const QString ERROR_CREATE_DIR = "Failed to create the destination folder: ";
const QString ERROR_NO_FILES = "No files were extracted.";

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

QString archiveError(archive *reader)
{
    const char *message = archive_error_string(reader);
    return message ? QString::fromLocal8Bit(message) : QStringLiteral("Unknown archive error.");
}

archive *openTarArchive(const QString &filePath, QString &errorMessage)
{
    archive *reader = archive_read_new();
    if (!reader) {
        errorMessage = ERROR_OPEN + QStringLiteral("failed to allocate archive reader.");
        return nullptr;
    }

    archive_read_support_filter_all(reader);
    archive_read_support_format_tar(reader);

    const QByteArray nativePath = QFile::encodeName(filePath);
    if (archive_read_open_filename(reader, nativePath.constData(), 65536) != ARCHIVE_OK) {
        errorMessage = ERROR_OPEN + archiveError(reader);
        archive_read_free(reader);
        return nullptr;
    }

    return reader;
}

QString safeOutputPath(const QString &rootPath, const QByteArray &archivePath, bool &valid)
{
    valid = false;
    if (archivePath.isEmpty()) {
        return QString();
    }

    QString relativePath = QFile::decodeName(archivePath);
    relativePath.replace(QLatin1Char('\\'), QLatin1Char('/'));

    if (relativePath.isEmpty() || QDir::isAbsolutePath(relativePath)) {
        return QString();
    }

    const QString cleanRoot = QDir::cleanPath(rootPath);
    const QString candidate = QDir::cleanPath(cleanRoot + QLatin1Char('/') + relativePath);

    if (candidate != cleanRoot && !candidate.startsWith(cleanRoot + QLatin1Char('/'))) {
        return QString();
    }

    valid = true;
    return candidate;
}

int countTarFiles(const QString &filePath, QString &errorMessage)
{
    archive *reader = openTarArchive(filePath, errorMessage);
    if (!reader) {
        return -1;
    }

    int count = 0;
    archive_entry *entry = nullptr;
    int result = ARCHIVE_OK;

    while ((result = archive_read_next_header(reader, &entry)) == ARCHIVE_OK) {
        if (archive_entry_filetype(entry) != AE_IFDIR) {
            ++count;
        }
        archive_read_data_skip(reader);
    }

    if (result != ARCHIVE_EOF) {
        errorMessage = ERROR_READ + archiveError(reader);
        archive_read_free(reader);
        return -1;
    }

    archive_read_free(reader);
    return count;
}

}

bool isSupportedTarArchive(const QString &filePath)
{
    const QString fileName = QFileInfo(filePath).fileName().toLower();
    static const QStringList suffixes = {
        QStringLiteral(".tar"),
        QStringLiteral(".tgz"),
        QStringLiteral(".tar.gz"),
        QStringLiteral(".tbz"),
        QStringLiteral(".tar.bz"),
        QStringLiteral(".tbz2"),
        QStringLiteral(".tar.bz2"),
        QStringLiteral(".tar.xz")
    };

    for (const QString &suffix : suffixes) {
        if (fileName.endsWith(suffix)) {
            return true;
        }
    }
    return false;
}

QStringList getTarArchiveContents(const QString &filePath, QString &errorMessage)
{
    errorMessage.clear();
    QStringList files;

    if (!fileExists(filePath, errorMessage)) {
        return files;
    }

    if (!isSupportedTarArchive(filePath)) {
        errorMessage = ERROR_UNSUPPORTED;
        return files;
    }

    archive *reader = openTarArchive(filePath, errorMessage);
    if (!reader) {
        return files;
    }

    archive_entry *entry = nullptr;
    int result = ARCHIVE_OK;

    while ((result = archive_read_next_header(reader, &entry)) == ARCHIVE_OK) {
        const char *pathname = archive_entry_pathname(entry);
        if (pathname && archive_entry_filetype(entry) != AE_IFDIR) {
            files << QFile::decodeName(pathname);
        }
        archive_read_data_skip(reader);
    }

    if (result != ARCHIVE_EOF) {
        errorMessage = ERROR_READ + archiveError(reader);
        files.clear();
    }

    archive_read_free(reader);
    return files;
}

bool extractTarArchive(const QString &filePath, const QString &outputPath,
                       QString &errorMessage,
                       ExtractProgressCallback progressCallback)
{
    errorMessage.clear();

    if (!fileExists(filePath, errorMessage)) {
        return false;
    }

    if (!isSupportedTarArchive(filePath)) {
        errorMessage = ERROR_UNSUPPORTED;
        return false;
    }

    if (outputPath.isEmpty()) {
        errorMessage = QStringLiteral("No output folder was selected.");
        return false;
    }

    if (!createOutputDir(outputPath, errorMessage)) {
        return false;
    }

    const QString rootPath = QDir::cleanPath(QDir(outputPath).absolutePath());

    QString countError;
    const int totalFiles = countTarFiles(filePath, countError);
    if (totalFiles < 0) {
        errorMessage = countError;
        return false;
    }

    archive *reader = openTarArchive(filePath, errorMessage);
    if (!reader) {
        return false;
    }

    const int extractFlags = ARCHIVE_EXTRACT_TIME
        | ARCHIVE_EXTRACT_PERM
        | ARCHIVE_EXTRACT_ACL
        | ARCHIVE_EXTRACT_FFLAGS
        | ARCHIVE_EXTRACT_SECURE_SYMLINKS
        | ARCHIVE_EXTRACT_SECURE_NODOTDOT;

    int currentFile = 0;
    int extractedCount = 0;
    int failedCount = 0;
    bool anySuccess = false;
    bool fatalError = false;
    archive_entry *entry = nullptr;
    int result = ARCHIVE_OK;

    while ((result = archive_read_next_header(reader, &entry)) == ARCHIVE_OK) {
        const char *pathname = archive_entry_pathname(entry);
        if (!pathname) {
            archive_read_data_skip(reader);
            continue;
        }

        const QString entryName = QFile::decodeName(pathname);
        const mode_t entryType = archive_entry_filetype(entry);

        bool validPath = false;
        const QString targetPath = safeOutputPath(rootPath, QByteArray(pathname), validPath);

        QByteArray hardlinkPath;
        const char *hardlink = archive_entry_hardlink(entry);
        if (validPath && hardlink) {
            bool validHardlink = false;
            const QString hardlinkTarget = safeOutputPath(rootPath, QByteArray(hardlink), validHardlink);
            if (!validHardlink) {
                validPath = false;
            } else {
                hardlinkPath = QFile::encodeName(hardlinkTarget);
                archive_entry_set_hardlink(entry, hardlinkPath.constData());
            }
        }

        if (!validPath) {
            qWarning() << "Skipping unsafe tar path:" << entryName;
            if (entryType != AE_IFDIR) {
                ++currentFile;
                if (progressCallback) {
                    progressCallback(currentFile, totalFiles, entryName);
                }
            }
            archive_read_data_skip(reader);
            continue;
        }

        const QByteArray nativeTargetPath = QFile::encodeName(targetPath);
        archive_entry_set_pathname(entry, nativeTargetPath.constData());

        const int extractResult = archive_read_extract(reader, entry, extractFlags);
        if (extractResult == ARCHIVE_OK || extractResult == ARCHIVE_WARN) {
            ++extractedCount;
            anySuccess = true;
        } else {
            ++failedCount;
            qWarning() << "Failed to extract tar entry:" << entryName << archiveError(reader);
            if (extractResult == ARCHIVE_FATAL) {
                fatalError = true;
                break;
            }
        }

        if (entryType != AE_IFDIR) {
            ++currentFile;
            if (progressCallback) {
                progressCallback(currentFile, totalFiles, entryName);
            }
        }
    }

    if (fatalError) {
        errorMessage = ERROR_READ + archiveError(reader);
        archive_read_free(reader);
        return false;
    }

    if (result != ARCHIVE_EOF) {
        errorMessage = ERROR_READ + archiveError(reader);
        archive_read_free(reader);
        return false;
    }

    archive_read_free(reader);

    if (!anySuccess) {
        errorMessage = ERROR_NO_FILES;
        return false;
    }

    if (failedCount > 0) {
        qWarning() << failedCount << "tar file(s) failed to extract.";
    }

    return true;
}
