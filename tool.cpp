// ============================================================
// tool.cpp
// جميع عمليات الاستخراج وعرض المحتويات
// ============================================================

#include "tool.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "debughelper.h"
#include "progresshelper.h"

#include <QFileInfo>
#include <QDir>
#include <QApplication>
#include <QDebug>
#include <QThread>

#include <quazip5/JlCompress.h>

// ============================================================
// الثوابت (Constants)
// ============================================================
namespace {
const QString ERROR_FILE_NOT_FOUND = "File not found: ";
const QString ERROR_EXTRACT_FAILED = "Extraction failed: The file is corrupt or unsupported.";
const QString ERROR_LIST_FAILED = "Failed to display content: the file is corrupted or unsupported.";
const QString ERROR_CREATE_DIR = "Failed to create the destination folder: ";
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
bool extractArchiveWithProgress(const QString &filePath, const QString &outputPath, QString &errorMessage, MainWindow *mainWindow)
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

    QString nativePath = QDir::toNativeSeparators(filePath);
    QString nativeOutput = QDir::toNativeSeparators(finalOutputPath);

    //  1. تهيئة StatusBar و ProgressBar
    if (mainWindow) {
        showExtractStart(mainWindow->getStatusBar());

        //  احصل على عدد الملفات مسبقاً (للتقدم)
        QStringList allFiles = JlCompress::getFileList(nativePath);
        int total = allFiles.size();
        if (total > 0) {
            initProgressBar(mainWindow->getProgressBar(), total);
        } else {
            initProgressBar(mainWindow->getProgressBar(), 1);
        }
    }

    //  2. استخدم extractDir (الأساسي)
    QStringList extractedFiles = JlCompress::extractDir(nativePath, nativeOutput);

    if (!extractedFiles.isEmpty()) {
        //  3. تحديث ProgressBar إلى 100%
        if (mainWindow) {
            setProgressValue(mainWindow->getProgressBar(), 100);  //  قفز إلى 100%
            QApplication::processEvents();  // تحديث الواجهة
            QThread::msleep(300);  // انتظر نصف ثانية (لترى التقدم)
            showExtractEnd(mainWindow->getStatusBar(), extractedFiles.size());
            resetProgressBar(mainWindow->getProgressBar());
        }
        debugQuaZipResult("extractDir", true, QString::number(extractedFiles.size()) + " files");
        debugFunctionEnd("extractArchiveWithProgress");
        return true;
    }

    //  4. إذا فشل extractDir، استخدم extractFile (كحل بديل)
    debugError("extractDir فشل، نحاول extractFile");

    QStringList allFiles = JlCompress::getFileList(nativePath);
    if (allFiles.isEmpty()) {
        errorMessage = "The file is corrupt or unsupported.";
        debugError(errorMessage);
        if (mainWindow) resetProgressBar(mainWindow->getProgressBar());
        debugFunctionEnd("extractArchiveWithProgress");
        return false;
    }

    int total = allFiles.size();
    int extractedCount = 0;

    if (mainWindow) {
        initProgressBar(mainWindow->getProgressBar(), total);
    }

    for (const QString &singleFile : allFiles) {
        QApplication::processEvents();

        QString result = JlCompress::extractFile(nativePath, singleFile, nativeOutput);
        if (!result.isEmpty()) {
            extractedCount++;
            if (mainWindow) {
                int percent = (extractedCount * 100) / total;
                QString statusMsg = QString(" %1 (%2%%) - %3/%4")
                                        .arg(singleFile)
                                        .arg(percent)
                                        .arg(extractedCount)
                                        .arg(total);
                mainWindow->getStatusBar()->showMessage(statusMsg, 0);
                setProgressValue(mainWindow->getProgressBar(), extractedCount);
            }
            debugExtractFile(singleFile, true);
        } else {
            debugExtractFile(singleFile, false);
        }
    }

    if (mainWindow) {
        resetProgressBar(mainWindow->getProgressBar());
    }

    if (extractedCount == 0) {
        errorMessage = "Failed to extract any file.";
        debugError(errorMessage);
        debugFunctionEnd("extractArchiveWithProgress");
        return false;
    }

    if (mainWindow) {
        showExtractEnd(mainWindow->getStatusBar(), extractedCount);
    }

    debugQuaZipResult("extractFile", true, QString::number(extractedCount) + " files extracted");
    debugFunctionEnd("extractArchiveWithProgress");
    return true;
}
