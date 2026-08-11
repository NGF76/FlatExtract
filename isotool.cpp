// ============================================================
// isotool.cpp
// دوال التعامل مع ملفات ISO (Xbox 360 / XDVDFS) + استخراج حزم STFS تلقائياً
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
// 4. استخراج ملف واحد من ISO (بالمسار الكامل)
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

    std::vector<uint8_t> data;
    if (!iso.ExtractBySector(match->startSector, match->fileSize, data)) {
        errorMessage = "خطأ في قراءة ملف ISO";
        debugError(errorMessage);
        debugFunctionEnd("extractFileFromIso");
        return false;
    }

    QString baseName = fileName;
    int lastSep = baseName.lastIndexOf('\\');
    if (lastSep >= 0) baseName = baseName.mid(lastSep + 1);

    QString fullOutputPath = outputPath + "/" + baseName;
    std::ofstream out(fullOutputPath.toStdString(), std::ios::binary);
    if (!out) {
        errorMessage = "فشل في إنشاء الملف الناتج: " + fullOutputPath;
        debugError(errorMessage);
        debugFunctionEnd("extractFileFromIso");
        return false;
    }
    out.write((const char*)data.data(), (std::streamsize)data.size());
    out.close();

    debugQuaZipResult("extractFileFromIso", true, fileName);
    debugFunctionEnd("extractFileFromIso");
    return true;
}

// ============================================================
// 5. استخراج كل الملفات من ISO + استخراج حزم STFS تلقائياً
// ============================================================

bool extractAllFromIso(const QString &isoPath, const QString &outputPath, QString &errorMessage)
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

    // ADD these two lines:
    int xdvdfsTotal = 0;
    for (const auto &e : entries) if (!e.isDirectory) xdvdfsTotal++;
    int currentIndex = 0;   // ADD

    bool anySuccess = false;
    uint64_t totalBytesWritten = 0;

    for (const auto &e : entries) {
        if (e.isDirectory) continue;

        std::vector<uint8_t> data;
        if (!iso.ExtractBySector(e.startSector, e.fileSize, data)) {
            qDebug() << "❌ Failed to extract:" << QString::fromStdString(e.path);
            continue;
        }

        QString relPath = QString::fromStdString(e.path);
        relPath.replace('\\', '/');
        QString fullOutputPath = outputPath + "/" + relPath;

        QFileInfo fi(fullOutputPath);
        if (!QDir().mkpath(fi.absolutePath())) {
            qDebug() << "❌ Failed to create directory for:" << relPath;
            continue;
        }

        std::ofstream out(fullOutputPath.toStdString(), std::ios::binary);
        if (!out) {
            qDebug() << "❌ Failed to create output file:" << fullOutputPath;
            continue;
        }
        out.write((const char*)data.data(), (std::streamsize)data.size());
        out.close();

        totalBytesWritten += data.size();
        anySuccess = true;
        qDebug() << "✅ Extracted:" << relPath << "(" << data.size() << "bytes)";

    }

    qDebug() << "Total XDVDFS bytes written:" << totalBytesWritten;

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

        // Extract into a sibling folder named "<file>_extracted"
        QString stfsOutDir = fullOutputPath + "_extracted";
        auto stfsFiles = stfs.ListAllFiles();

        int stfsSuccessCount = 0;
        uint64_t stfsBytesWritten = 0;



        for (const auto &sf : stfsFiles) {
            if (sf.isDirectory) continue;

            std::vector<uint8_t> data;
            if (!stfs.ExtractFile(sf, data)) {
                qDebug() << "  ❌ Failed to extract STFS entry:" << QString::fromStdString(sf.path);
                continue;
            }

            QString sfRelPath = QString::fromStdString(sf.path);
            sfRelPath.replace('\\', '/');
            QString sfFullPath = stfsOutDir + "/" + sfRelPath;

            QFileInfo sfInfo(sfFullPath);
            if (!QDir().mkpath(sfInfo.absolutePath())) {
                qDebug() << "  ❌ Failed to create directory for:" << sfRelPath;
                continue;
            }

            std::ofstream sfOut(sfFullPath.toStdString(), std::ios::binary);
            if (!sfOut) {
                qDebug() << "  ❌ Failed to create output file:" << sfFullPath;
                continue;
            }
            sfOut.write((const char*)data.data(), (std::streamsize)data.size());
            sfOut.close();

            stfsSuccessCount++;
            stfsBytesWritten += data.size();
        }

        qDebug() << "  ✅ STFS extracted:" << stfsSuccessCount << "files,"
                 << stfsBytesWritten << "bytes ->" << stfsOutDir;
    }
    // ---- END STFS auto-extraction ----

    debugQuaZipResult("extractAllFromIso", true, "All files extracted");
    debugFunctionEnd("extractAllFromIso");
    return true;
}
