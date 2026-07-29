#include "tool.h"
#include <QProcess>
#include <QFileInfo>
#include <QDebug>
#include <QDir>


// ============================================================
// دالة البحث عن الأمر المناسب
// ============================================================
QString findArchiveExecutable()
{
    QStringList possibleNames = {"7z", "7za", "7zr", "unzip", "tar", "gunzip"};
    QStringList searchPaths = {"/usr/bin/", "/usr/local/bin/"};

    for (const QString &name : possibleNames) {
        for (const QString &path : searchPaths) {
            QFileInfo file(path + name);
            if (file.exists() && file.isExecutable()) {
                return file.absoluteFilePath();
            }
        }
    }
    return "";
}

// ============================================================
// دالة استخراج قائمة الملفات
// ============================================================
QStringList getArchiveContents(const QString &filePath, QString &errorMessage)
{
    QStringList files;
    errorMessage.clear();

    if (!QFileInfo(filePath).exists()) {
        errorMessage = "الملف غير موجود: " + filePath;
        return files;
    }

    QString suffix = QFileInfo(filePath).suffix().toLower();
    QProcess process;
    QStringList arguments;

    // ========================================
    // اختيار الأمر حسب نوع الملف
    // ========================================
    if (suffix == "zip") {
        arguments << "-l" << filePath;
        process.start("unzip", arguments);
    }
    else if (suffix == "7z") {
        arguments << "l" << filePath;
        process.start("7z", arguments);
    }
    else if (suffix == "gz" && filePath.endsWith(".tar.gz")) {
        arguments << "-tzf" << filePath;
        process.start("tar", arguments);
    }
    else if (suffix == "bz2" && filePath.endsWith(".tar.bz2")) {
        arguments << "-tjf" << filePath;
        process.start("tar", arguments);
    }
    else if (suffix == "xz" && filePath.endsWith(".tar.xz")) {
        arguments << "-tJf" << filePath;
        process.start("tar", arguments);
    }
    else if (suffix == "tar") {
        arguments << "-tf" << filePath;
        process.start("tar", arguments);
    }
    else {
        errorMessage = "نوع الملف غير مدعوم: " + suffix;
        return files;
    }

    // تنفيذ الأمر
    process.waitForFinished(-1);
    QString output = process.readAllStandardOutput();
    QString error = process.readAllStandardError();

    if (process.exitCode() != 0) {
        errorMessage = "خطأ في تنفيذ الأمر: " + error;
        return files;
    }

    // ========================================
    // تحليل الناتج حسب نوع الملف
    // ========================================
    if (suffix == "zip") {
        QStringList lines = output.split("\n");
        for (const QString &line : lines) {
            if (line.contains(".") && !line.contains("---") && !line.contains("Archive")) {
                QStringList parts = line.split(" ", Qt::SkipEmptyParts);
                if (parts.size() >= 4) {
                    files << parts.last();
                }
            }
        }
    }
    else if (suffix == "7z") {
        QStringList lines = output.split("\n");
        bool startReading = false;
        for (const QString &line : lines) {
            if (line.contains("----")) {
                startReading = true;
                continue;
            }
            if (!startReading || line.trimmed().isEmpty()) continue;
            if (line.contains("Date") || line.contains("----")) continue;

            QStringList parts = line.split(" ", Qt::SkipEmptyParts);
            if (parts.size() >= 6) {
                QString fileName;
                for (int i = 5; i < parts.size(); ++i) {
                    fileName += parts[i];
                    if (i < parts.size() - 1) fileName += " ";
                }
                files << fileName;
            }
        }
    }
    else {
        // tar: كل سطر هو اسم ملف
        QStringList lines = output.split("\n");
        for (const QString &line : lines) {
            if (!line.trimmed().isEmpty()) {
                files << line.trimmed();
            }
        }
    }

    return files;
}

// ============================================================
// دالة استخراج الملفات
// ============================================================
bool extractArchive(const QString &filePath, const QString &outputPath, QString &errorMessage)
{
    errorMessage.clear();

    if (!QFileInfo(filePath).exists()) {
        errorMessage = "الملف غير موجود: " + filePath;
        return false;
    }

    // أنشئ مجلد الإخراج إذا لم يكن موجوداً
    QDir().mkpath(outputPath);

    QString suffix = QFileInfo(filePath).suffix().toLower();
    QProcess process;
    QStringList arguments;

    // ========================================
    // اختيار أمر الاستخراج حسب نوع الملف
    // ========================================
    if (suffix == "zip") {
        arguments << filePath << "-d" << outputPath;
        process.start("unzip", arguments);
    }
    else if (suffix == "7z") {
        arguments << "x" << filePath << "-o" << outputPath;
        process.start("7z", arguments);
    }
    else if (suffix == "gz" && filePath.endsWith(".tar.gz")) {
        arguments << "-xzf" << filePath << "-C" << outputPath;
        process.start("tar", arguments);
    }
    else if (suffix == "bz2" && filePath.endsWith(".tar.bz2")) {
        arguments << "-xjf" << filePath << "-C" << outputPath;
        process.start("tar", arguments);
    }
    else if (suffix == "xz" && filePath.endsWith(".tar.xz")) {
        arguments << "-xJf" << filePath << "-C" << outputPath;
        process.start("tar", arguments);
    }
    else if (suffix == "tar") {
        arguments << "-xf" << filePath << "-C" << outputPath;
        process.start("tar", arguments);
    }
    else if (suffix == "gz" && !filePath.endsWith(".tar.gz")) {
        // ملف .gz منفرد
        arguments << "-d" << filePath;
        process.start("gunzip", arguments);
        // لا يمكن تحديد مجلد الإخراج لـ gunzip، سيكون في نفس المجلد
    }
    else {
        errorMessage = "نوع الملف غير مدعوم: " + suffix;
        return false;
    }

    // تنفيذ الأمر
    process.waitForFinished(-1);
    QString error = process.readAllStandardError();

    if (process.exitCode() != 0) {
        errorMessage = "خطأ في الاستخراج: " + error;
        return false;
    }

    return true;
}
