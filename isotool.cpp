// ============================================================
// isotool.cpp
// دوال التعامل مع ملفات ISO (Xbox 360 / XDVDFS) + استخراج حزم STFS تلقائياً
// (نسخة محسّنة للذاكرة: كتابة مباشرة على القرص بدل تخزين كل ملف بالكامل في RAM)
// ============================================================

#include "isotool.h"
#include "debughelper.h"

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
const QString ERROR_FILE_NOT_FOUND = "الملف غير موجود: ";
const QString ERROR_OPEN_ISO = "فشل في فتح ملف ISO: ";
const QString ERROR_READ_CONTENTS = "فشل في قراءة محتويات ISO";
const QString ERROR_NO_FILES = "لم يتم استخراج أي ملف";
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
        errorMessage = "فشل في إنشاء مجلد الوجهة: " + outputPath;
        return false;
    }
    return true;
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

    IsoReader iso;
    if (!iso.Open(isoPath.toStdString())) {
        errorMessage = ERROR_OPEN_ISO + isoPath;
        debugError(errorMessage);
        debugFunctionEnd("getIsoContents");
        return QStringList();
    }

    auto entries = iso.ListAllFiles();

    QStringList files;
    for (const auto &e : entries) {
        files << QString::fromStdString(e.path);
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
        errorMessage = "الملف غير موجود في ISO: " + fileName;
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
        errorMessage = "خطأ في قراءة/كتابة ملف ISO";
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

    for (const auto &e : entries) {
        if (e.isDirectory) continue;

        QString relPath = QString::fromStdString(e.path);
        relPath.replace('\\', '/');
        QString fullOutputPath = outputPath + "/" + relPath;

        QFileInfo fi(fullOutputPath);
        if (!QDir().mkpath(fi.absolutePath())) {
            qDebug() << "❌ Failed to create directory for:" << relPath;
            currentIndex++;
            if (progressCallback) progressCallback(currentIndex, xdvdfsTotal, relPath);
            continue;
        }

        // Streaming write directly to disk - bounded memory regardless of file size.
        if (!iso.ExtractBySectorToDisk(e.startSector, e.fileSize, fullOutputPath.toStdString())) {
            qDebug() << "❌ Failed to extract:" << relPath;
            currentIndex++;
            if (progressCallback) progressCallback(currentIndex, xdvdfsTotal, relPath);
            continue;
        }

        anySuccess = true;
        qDebug() << "✅ Extracted:" << relPath << "(" << e.fileSize << "bytes)";

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

        qDebug() << "📦 STFS package detected:" << relPath
                 << "-" << QString::fromStdString(stfs.GetDisplayName());

        QString stfsOutDir = fullOutputPath + "_extracted";
        auto stfsFiles = stfs.ListAllFiles();

        int stfsTotal = 0;
        for (const auto &sf : stfsFiles) if (!sf.isDirectory) stfsTotal++;
        int stfsIndex = 0;

        int stfsSuccessCount = 0;

        for (const auto &sf : stfsFiles) {
            if (sf.isDirectory) continue;

            QString sfRelPath = QString::fromStdString(sf.path);
            sfRelPath.replace('\\', '/');
            QString sfFullPath = stfsOutDir + "/" + sfRelPath;

            QFileInfo sfInfo(sfFullPath);
            if (!QDir().mkpath(sfInfo.absolutePath())) {
                qDebug() << "  ❌ Failed to create directory for:" << sfRelPath;
                stfsIndex++;
                if (progressCallback) progressCallback(stfsIndex, stfsTotal, sfRelPath);
                continue;
            }

            // Streaming write directly to disk - bounded memory regardless of file size.
            if (!stfs.ExtractFileToDisk(sf, sfFullPath.toStdString())) {
                qDebug() << "  ❌ Failed to extract STFS entry:" << sfRelPath;
                stfsIndex++;
                if (progressCallback) progressCallback(stfsIndex, stfsTotal, sfRelPath);
                continue;
            }

            stfsSuccessCount++;

            stfsIndex++;
            if (progressCallback) progressCallback(stfsIndex, stfsTotal, sfRelPath);
        }

        qDebug() << "  ✅ STFS extracted:" << stfsSuccessCount << "files ->" << stfsOutDir;
    }
    // ---- END STFS auto-extraction ----

    debugQuaZipResult("extractAllFromIso", true, "All files extracted");
    debugFunctionEnd("extractAllFromIso");
    return true;
}
