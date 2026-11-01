// ============================================================
// isotool.cpp
// دوال التعامل مع ملفات ISO (Xbox 360 / XDVDFS) + استخراج حزم STFS تلقائياً
// (نسخة محسّنة للذاكرة: كتابة مباشرة على القرص بدل تخزين كل ملف بالكامل في RAM)
// ============================================================

#include "isotool.h"
#include "debughelper.h"
#include "gameiso.h"

#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <fstream>
#include "iso_reader.h"
#include "xex_parser.h"
#include "stfs_reader.h"

// ============================================================
// 1. ثوابت (Constants)
// ============================================================

namespace {
const QString ERROR_FILE_NOT_FOUND = "File not found: ";
const QString ERROR_OPEN_ISO = "Failed to open ISO file: ";
const QString ERROR_READ_CONTENTS = "Failed to read ISO contents.";
const QString ERROR_NO_FILES = "No file was extracted.";
}

// ============================================================
// 2. دوال مساعدة (Helper Functions)
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
        errorMessage = "Failed to create the destination folder: " + outputPath;
        return false;
    }
    return true;
}

// XDVDFS is used by both Xbox 360 and original Xbox images.
static QStringList listXdvdfsContents(const QString &isoPath, QString &errorMessage)
{
    IsoReader iso;
    if (!iso.Open(isoPath.toStdString())) {
        errorMessage = ERROR_OPEN_ISO + isoPath;
        return QStringList();
    }

    auto entries = iso.ListAllFiles();

    QStringList files;
    for (const auto &e : entries) {
        files << QString::fromStdString(e.path);
    }
    return files;
}

// ============================================================
// 3. عرض محتويات ISO
// ============================================================

QStringList getIsoContents(const QString &isoPath, QString &errorMessage)
{
    debugFunctionStart("getIsoContents");
    errorMessage.clear();

    if (!fileExists(isoPath, errorMessage)) {
        debugError(errorMessage);
        debugFunctionEnd("getIsoContents");
        return QStringList();
    }

    const IsoImageFormat format = detectIsoImageFormat(isoPath);
    QStringList files;

    switch (format) {
    case IsoImageFormat::Iso9660:
        files = listIso9660Contents(isoPath, errorMessage);
        break;

    case IsoImageFormat::Udf: {
        files = listUdfContents(isoPath, errorMessage);

        // Hybrid images may contain a usable ISO 9660 file system even when
        // the UDF reader cannot handle their partition layout.
        if (!errorMessage.isEmpty() && hasIso9660FileSystem(isoPath)) {
            const QString udfError = errorMessage;
            errorMessage.clear();
            files = listIso9660Contents(isoPath, errorMessage);
            if (!errorMessage.isEmpty()) {
                errorMessage = udfError + QLatin1Char('\n') + errorMessage;
            }
        }
        break;
    }

    case IsoImageFormat::GameCube:
        files = listGameCubeContents(isoPath, errorMessage);
        break;

    case IsoImageFormat::XboxXdvdfs:
    case IsoImageFormat::Unknown:
        // Preserve the original XDVDFS path, including images whose volume
        // descriptor is not at one of the standard offsets.
        files = listXdvdfsContents(isoPath, errorMessage);
        break;
    }

    debugFileList("Files in ISO", files);
    debugFunctionEnd("getIsoContents");
    return files;
}

// ============================================================
// 4. استخراج ملف واحد من ISO (بالمسار الكامل) - كتابة مباشرة على القرص
// ============================================================

bool extractFileFromIso(const QString &isoPath, const QString &fileName, const QString &outputPath, QString &errorMessage)
{
    debugFunctionStart("extractFileFromIso");
    errorMessage.clear();

    if (!fileExists(isoPath, errorMessage)) {
        debugError(errorMessage);
        debugFunctionEnd("extractFileFromIso");
        return false;
    }

    if (!createOutputDir(outputPath, errorMessage)) {
        debugError(errorMessage);
        debugFunctionEnd("extractFileFromIso");
        return false;
    }

    IsoReader iso;
    if (!iso.Open(isoPath.toStdString())) {
        errorMessage = ERROR_OPEN_ISO + isoPath;
        debugError(errorMessage);
        debugFunctionEnd("extractFileFromIso");
        return false;
    }

    auto entries = iso.ListAllFiles();
    std::string target = fileName.toStdString();

    const IsoFileEntry *match = nullptr;
    for (const auto &e : entries) {
        if (!e.isDirectory && e.path == target) {
            match = &e;
            break;
        }
    }

    if (!match) {
        errorMessage = "The file is not present in the ISO: " + fileName;
        debugError(errorMessage);
        debugFunctionEnd("extractFileFromIso");
        return false;
    }

    QString baseName = fileName;
    int lastSep = baseName.lastIndexOf('\\');
    if (lastSep >= 0) baseName = baseName.mid(lastSep + 1);

    QString fullOutputPath = outputPath + "/" + baseName;

    // Streaming write: bounded memory regardless of file size.
    if (!iso.ExtractBySectorToDisk(match->startSector, match->fileSize, fullOutputPath.toStdString())) {
        errorMessage = "ISO file read/write error";
        debugError(errorMessage);
        debugFunctionEnd("extractFileFromIso");
        return false;
    }

    debugQuaZipResult("extractFileFromIso", true, fileName);
    debugFunctionEnd("extractFileFromIso");
    return true;
}

// ============================================================
// 5. استخراج كل الملفات من ISO + استخراج حزم STFS تلقائياً
//    (كتابة مباشرة على القرص لكل ملف - لا يتم تحميل أي ملف بالكامل في RAM)
// ============================================================

bool extractAllFromIso(const QString &isoPath, const QString &outputPath,
                       QString &errorMessage,
                       ExtractProgressCallback progressCallback)
{
    debugFunctionStart("extractAllFromIso");
    errorMessage.clear();

    if (!fileExists(isoPath, errorMessage)) {
        debugError(errorMessage);
        debugFunctionEnd("extractAllFromIso");
        return false;
    }

    if (!createOutputDir(outputPath, errorMessage)) {
        debugError(errorMessage);
        debugFunctionEnd("extractAllFromIso");
        return false;
    }

    const IsoImageFormat format = detectIsoImageFormat(isoPath);

    if (format == IsoImageFormat::Iso9660) {
        const bool ok = extractIso9660Image(isoPath, outputPath, errorMessage, progressCallback);
        if (!ok) debugError(errorMessage);
        debugFunctionEnd("extractAllFromIso");
        return ok;
    }

    if (format == IsoImageFormat::GameCube) {
        const bool ok = extractGameCubeImage(isoPath, outputPath, errorMessage, progressCallback);
        if (!ok) debugError(errorMessage);
        debugFunctionEnd("extractAllFromIso");
        return ok;
    }

    if (format == IsoImageFormat::Udf) {
        QString udfError;
        if (extractUdfImage(isoPath, outputPath, udfError, progressCallback)) {
            debugFunctionEnd("extractAllFromIso");
            return true;
        }

        // UDF bridge images may still be readable through their ISO 9660
        // file system (for example when libudfread is not installed).
        if (hasIso9660FileSystem(isoPath)) {
            errorMessage.clear();
            if (extractIso9660Image(isoPath, outputPath, errorMessage, progressCallback)) {
                debugFunctionEnd("extractAllFromIso");
                return true;
            }

            if (!errorMessage.isEmpty()) {
                errorMessage = udfError + QLatin1Char('\n') + errorMessage;
            } else {
                errorMessage = udfError;
            }
        } else {
            errorMessage = udfError;
        }

        debugError(errorMessage);
        debugFunctionEnd("extractAllFromIso");
        return false;
    }

    // Xbox 360 XDVDFS, original Xbox images, and legacy images that are only
    // recognized by the XDVDFS reader continue through the original path.
    IsoReader iso;
    if (!iso.Open(isoPath.toStdString())) {
        errorMessage = ERROR_OPEN_ISO + isoPath;
        debugError(errorMessage);
        debugFunctionEnd("extractAllFromIso");
        return false;
    }

    auto entries = iso.ListAllFiles();

    int xdvdfsTotal = 0;
    for (const auto &e : entries) if (!e.isDirectory) xdvdfsTotal++;
    int currentIndex = 0;

    bool anySuccess = false;
    int stfsCompleted = 0;

    for (const auto &e : entries) {
        if (e.isDirectory) continue;

        QString relPath = QString::fromStdString(e.path);
        relPath.replace('\\', '/');
        QString fullOutputPath = outputPath + "/" + relPath;

        QFileInfo fi(fullOutputPath);
        if (!QDir().mkpath(fi.absolutePath())) {
            qDebug() << " Failed to create directory for:" << relPath;
            currentIndex++;
            if (progressCallback) progressCallback(currentIndex, xdvdfsTotal, relPath);
            continue;
        }

        // Streaming write directly to disk - bounded memory regardless of file size.
        if (!iso.ExtractBySectorToDisk(e.startSector, e.fileSize, fullOutputPath.toStdString())) {
            qDebug() << " Failed to extract:" << relPath;
            currentIndex++;
            if (progressCallback) progressCallback(currentIndex, xdvdfsTotal, relPath);
            continue;
        }

        anySuccess = true;
        qDebug() << " Extracted Done:" << relPath << "(" << e.fileSize << "bytes)";

        currentIndex++;
        if (progressCallback) progressCallback(currentIndex, xdvdfsTotal, relPath);
    }

    if (!anySuccess) {
        errorMessage = ERROR_NO_FILES;
        debugError(errorMessage);
        debugFunctionEnd("extractAllFromIso");
        return false;
    }

    // ---- Auto-detect and recursively extract any STFS packages ----
    for (const auto &e : entries) {
        if (e.isDirectory) continue;

        QString relPath = QString::fromStdString(e.path);
        relPath.replace('\\', '/');
        QString fullOutputPath = outputPath + "/" + relPath;

        StfsReader stfs;
        if (!stfs.Open(fullOutputPath.toStdString())) {
            continue; // not an STFS package, skip silently
        }

        qDebug() << " STFS package detected:" << relPath
                 << "-" << QString::fromStdString(stfs.GetDisplayName());

        QString stfsOutDir = fullOutputPath + "_extracted";
        auto stfsFiles = stfs.ListAllFiles();

        int stfsTotal = 0;
        for (const auto &sf : stfsFiles) if (!sf.isDirectory) stfsTotal++;
        int stfsIndex = 0;
        const int stfsBase = xdvdfsTotal + stfsCompleted;
        const int stfsOverallTotal = stfsBase + stfsTotal;

        int stfsSuccessCount = 0;

        for (const auto &sf : stfsFiles) {
            if (sf.isDirectory) continue;

            QString sfRelPath = QString::fromStdString(sf.path);
            sfRelPath.replace('\\', '/');
            QString sfFullPath = stfsOutDir + "/" + sfRelPath;

            QFileInfo sfInfo(sfFullPath);
            if (!QDir().mkpath(sfInfo.absolutePath())) {
                qDebug() << "   Failed to create directory for:" << sfRelPath;
                stfsIndex++;
                if (progressCallback) progressCallback(stfsBase + stfsIndex, stfsOverallTotal, sfRelPath);
                continue;
            }

            // Streaming write directly to disk - bounded memory regardless of file size.
            if (!stfs.ExtractFileToDisk(sf, sfFullPath.toStdString())) {
                qDebug() << "   Failed to extract STFS entry:" << sfRelPath;
                stfsIndex++;
                if (progressCallback) progressCallback(stfsBase + stfsIndex, stfsOverallTotal, sfRelPath);
                continue;
            }

            stfsSuccessCount++;

            stfsIndex++;
            if (progressCallback) progressCallback(stfsBase + stfsIndex, stfsOverallTotal, sfRelPath);
        }

        stfsCompleted += stfsTotal;
        qDebug() << "   STFS extracted:" << stfsSuccessCount << "files ->" << stfsOutDir;
    }
    // ---- END STFS auto-extraction ----

    debugQuaZipResult("extractAllFromIso", true, "All files extracted");
    debugFunctionEnd("extractAllFromIso");
    return true;
}
