// ============================================================
// tool.cpp
// جميع عمليات الاستخراج وعرض المحتويات
// ============================================================

#include "tool.h"
#include "debughelper.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QApplication>
#include <QDebug>

#include <quazip5/JlCompress.h>
#include <quazip5/quazip.h>
#include <quazip5/quazipfile.h>
#include <quazip5/quazipfileinfo.h>

// ============================================================
// الثوابت (Constants)
// ============================================================
namespace {
const QString ERROR_FILE_NOT_FOUND = "File not found: ";
const QString ERROR_EXTRACT_FAILED = "Extraction failed: The file is corrupt or unsupported.";
const QString ERROR_LIST_FAILED = "Failed to display content: the file is corrupted or unsupported.";
const QString ERROR_CREATE_DIR = "Failed to create the destination folder: ";

bool isZipDirectoryEntry(const QuaZipFileInfo64 &info)
{
    if (info.name.endsWith('/')) {
        return true;
    }

    const quint32 dosAttributes = info.externalAttr & 0xFF;
    if ((dosAttributes & 0x10) != 0) {
        return true;
    }

    const quint32 unixMode = info.externalAttr >> 16;
    return (unixMode & 0xF000) == 0x4000;
}

QString zipOutputPath(const QString &rootPath, const QString &entryName, bool &valid)
{
    valid = false;
    const QString cleanRoot = QDir::cleanPath(rootPath);
    const QString candidate = QDir::cleanPath(cleanRoot + QLatin1Char('/') + entryName);

    if (candidate != cleanRoot && !candidate.startsWith(cleanRoot + QLatin1Char('/'))) {
        return QString();
    }

    valid = true;
    return candidate;
}
}

// ============================================================
// دوال مساعدة (Helper Functions)
// ============================================================

static bool fileExists(const QString &filePath, QString &errorMessage)
{
    if (!QFileInfo(filePath).exists()) {
        errorMessage = ERROR_FILE_NOT_FOUND + filePath;
        return false;
    }
    return true;
}

static bool createOutputDir(const QString &outputPath, QString &errorMessage)
{
    if (!QDir().mkpath(outputPath)) {
        errorMessage = ERROR_CREATE_DIR + outputPath;
        return false;
    }
    return true;
}

// ============================================================
// عرض محتويات الملف المضغوط
// ============================================================
QStringList getArchiveContents(const QString &filePath, QString &errorMessage)
{
    debugFunctionStart("getArchiveContents");
    errorMessage.clear();

    // 1. التحقق من وجود الملف
    if (!fileExists(filePath, errorMessage)) {
        debugError(errorMessage);
        debugFunctionEnd("getArchiveContents");
        return QStringList();
    }

    // 2. قراءة المحتويات
    QString nativePath = QDir::toNativeSeparators(filePath);
    debugVariable("Native Path", nativePath);

    QStringList files = JlCompress::getFileList(nativePath);

    if (files.isEmpty()) {
        errorMessage = "The file is empty, corrupt, or unsupported.";
        debugError(errorMessage);
        debugFunctionEnd("getArchiveContents");
        return QStringList();
    }

    debugFileList("Files in archive", files);
    debugFunctionEnd("getArchiveContents");

    return files;
}

// ============================================================
// استخراج الملفات (بدون تقدم)
// ============================================================
bool extractArchive(const QString &filePath, const QString &outputPath, QString &errorMessage)
{
    debugFunctionStart("extractArchive");
    debugVariable("filePath", filePath);
    debugVariable("outputPath", outputPath);

    errorMessage.clear();

    // 1. تحديد مجلد الوجهة النهائي
    QString finalOutputPath = outputPath;
    if (finalOutputPath.isEmpty()) {
        finalOutputPath = QFileInfo(filePath).absolutePath() + "/extracted";
    }
    debugVariable("finalOutputPath", finalOutputPath);

    // 2. التحقق من وجود الملف
    if (!fileExists(filePath, errorMessage)) {
        debugError(errorMessage);
        debugFunctionEnd("extractArchive");
        return false;
    }

    // 3. إنشاء مجلد الوجهة
    if (!createOutputDir(finalOutputPath, errorMessage)) {
        debugError(errorMessage);
        debugFunctionEnd("extractArchive");
        return false;
    }

    // 4. تحويل المسارات
    QString nativePath = QDir::toNativeSeparators(filePath);
    QString nativeOutput = QDir::toNativeSeparators(finalOutputPath);
    debugVariable("nativePath", nativePath);
    debugVariable("nativeOutput", nativeOutput);

    // 5. محاولة extractDir أولاً
    QStringList extractedFiles = JlCompress::extractDir(nativePath, nativeOutput);

    if (!extractedFiles.isEmpty()) {
        debugQuaZipResult("extractDir", true, QString::number(extractedFiles.size()) + " files");
        debugFunctionEnd("extractArchive");
        return true;
    }

    // 6. إذا فشل extractDir، استخدم extractFile لكل ملف
    debugError("extractDir فشل، نحاول extractFile");

    QStringList allFiles = JlCompress::getFileList(nativePath);
    if (allFiles.isEmpty()) {
        errorMessage = "The file is corrupt or unsupported.";
        debugError(errorMessage);
        debugFunctionEnd("extractArchive");
        return false;
    }

    bool anyFileExtracted = false;
    for (const QString &singleFile : allFiles) {
        QApplication::processEvents();

        QString result = JlCompress::extractFile(nativePath, singleFile, nativeOutput);
        if (!result.isEmpty()) {
            anyFileExtracted = true;
            debugExtractFile(singleFile, true);
        } else {
            debugExtractFile(singleFile, false);
        }
    }

    if (!anyFileExtracted) {
        errorMessage = "Failed to extract any file from the archive.";
        debugError(errorMessage);
        debugFunctionEnd("extractArchive");
        return false;
    }

    debugQuaZipResult("extractFile", true, "Some files extracted");
    debugFunctionEnd("extractArchive");
    return true;
}

// ============================================================
// استخراج الملفات مع تقدم تدريجي (Progress)
// ============================================================
bool extractArchiveWithProgress(const QString &filePath, const QString &outputPath, QString &errorMessage,
                                ExtractProgressCallback progressCallback)
{
    debugFunctionStart("extractArchiveWithProgress");
    errorMessage.clear();

    if (!fileExists(filePath, errorMessage)) {
        debugError(errorMessage);
        debugFunctionEnd("extractArchiveWithProgress");
        return false;
    }

    QString finalOutputPath = outputPath;
    if (finalOutputPath.isEmpty()) {
        finalOutputPath = QFileInfo(filePath).absolutePath() + "/extracted";
    }

    if (!createOutputDir(finalOutputPath, errorMessage)) {
        debugError(errorMessage);
        debugFunctionEnd("extractArchiveWithProgress");
        return false;
    }

    const QString nativePath = QDir::toNativeSeparators(filePath);
    const QString nativeOutput = QDir::toNativeSeparators(finalOutputPath);
    const QString rootPath = QDir(nativeOutput).absolutePath();

    QuaZip zip(nativePath);
    if (!zip.open(QuaZip::mdUnzip)) {
        errorMessage = ERROR_EXTRACT_FAILED;
        debugError(errorMessage);
        debugFunctionEnd("extractArchiveWithProgress");
        return false;
    }

    const QList<QuaZipFileInfo64> entries = zip.getFileInfoList64();
    if (entries.isEmpty()) {
        errorMessage = "The file is empty, corrupt, or unsupported.";
        debugError(errorMessage);
        debugFunctionEnd("extractArchiveWithProgress");
        return false;
    }

    int totalFiles = 0;
    for (const QuaZipFileInfo64 &entry : entries) {
        if (!isZipDirectoryEntry(entry)) {
            ++totalFiles;
        }
    }

    int currentFile = 0;
    int extractedCount = 0;
    bool anySuccess = false;

    bool more = zip.goToFirstFile();
    while (more) {
        QuaZipFileInfo64 entry;
        if (!zip.getCurrentFileInfo(&entry)) {
            errorMessage = ERROR_EXTRACT_FAILED;
            debugError(errorMessage);
            debugFunctionEnd("extractArchiveWithProgress");
            return false;
        }

        bool validPath = false;
        const QString outputFilePath = zipOutputPath(rootPath, entry.name, validPath);

        if (!validPath) {
            if (!isZipDirectoryEntry(entry)) {
                ++currentFile;
                if (progressCallback) {
                    progressCallback(currentFile, totalFiles, entry.name);
                }
            }
            qWarning() << "Skipping unsafe ZIP path:" << entry.name;
            more = zip.goToNextFile();
            continue;
        }

        if (isZipDirectoryEntry(entry)) {
            if (QDir().mkpath(outputFilePath)) {
                anySuccess = true;
            } else {
                qWarning() << "Failed to create ZIP directory:" << outputFilePath;
            }
            more = zip.goToNextFile();
            continue;
        }

        ++currentFile;
        bool extracted = false;
        const QFileInfo outputInfo(outputFilePath);

        if (QDir().mkpath(outputInfo.absolutePath())) {
            QuaZipFile zipFile(&zip);
            if (zipFile.open(QIODevice::ReadOnly)) {
                QFile outFile(outputFilePath);
                if (outFile.open(QIODevice::WriteOnly)) {
                    extracted = true;
                    while (!zipFile.atEnd()) {
                        const QByteArray chunk = zipFile.read(64 * 1024);
                        if (chunk.isEmpty()) {
                            if (zipFile.getZipError() != 0) {
                                extracted = false;
                            }
                            break;
                        }
                        if (outFile.write(chunk) != chunk.size()) {
                            extracted = false;
                            break;
                        }
                    }
                    outFile.close();
                }
                zipFile.close();
            } else {
                qWarning() << "Failed to open ZIP entry:" << entry.name << zipFile.errorString();
            }
        }

        if (extracted) {
            const QFile::Permissions permissions = entry.getPermissions();
            if (permissions != QFileDevice::Permissions()) {
                QFile::setPermissions(outputFilePath, permissions);
            }
            ++extractedCount;
            anySuccess = true;
            debugExtractFile(entry.name, true);
        } else {
            QFile::remove(outputFilePath);
            debugExtractFile(entry.name, false);
        }

        if (progressCallback) {
            progressCallback(currentFile, totalFiles, entry.name);
        }

        more = zip.goToNextFile();
    }

    if (!anySuccess) {
        errorMessage = "Failed to extract any file.";
        debugError(errorMessage);
        debugFunctionEnd("extractArchiveWithProgress");
        return false;
    }

    debugQuaZipResult("extractArchiveWithProgress", true,
                      QString::number(extractedCount) + " files extracted");
    debugFunctionEnd("extractArchiveWithProgress");
    return true;
}
