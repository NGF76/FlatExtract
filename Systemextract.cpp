#include "Systemextract.h"
#include <QCoreApplication>
#include <QProcess>
#include <QDebug>
#include <QFileInfo>
#include <QDir>

// ============================================================
// دالة مساعدة: البحث عن 7z في مجلد التطبيق أولاً ثم النظام
// ============================================================
QString find7zExecutable()
{
    // 1. البحث في مجلد التطبيق (للنسخة المدمجة)
    QString appDirPath = QCoreApplication::applicationDirPath();
    QStringList localPaths = {
        appDirPath + "/7z",          // بجانب الملف التنفيذي مباشرة
        appDirPath + "/usr/bin/7z"   // داخل هيكل AppImage
    };

    for (const QString &path : localPaths) {
        QFileInfo file(path);
        if (file.exists() && file.isExecutable()) {
            return file.absoluteFilePath();
        }
    }

    // 2. البحث في مسارات النظام (حل احتياطي)
    QStringList systemPaths = {"/usr/bin/7z", "/usr/local/bin/7z"};
    for (const QString &path : systemPaths) {
        QFileInfo file(path);
        if (file.exists() && file.isExecutable()) {
            return file.absoluteFilePath();
        }
    }

    return ""; // لم يتم العثور عليه
}

// ============================================================
// دالة الاستخراج
// ============================================================
bool extractWithSystemTool(const QString &filePath, const QString &outputPath, QString &errorMessage)
{
    errorMessage.clear();

    if (!QFileInfo(filePath).exists()) {
        errorMessage = "The File Does Not Exist: " + filePath;
        return false;
    }

    // ✅ البحث عن 7z
    QString sevenZipPath = find7zExecutable();
    if (sevenZipPath.isEmpty()) {
        errorMessage = "7z executable not found. Please install p7zip-full.";
        return false;
    }

    QDir().mkpath(outputPath);

    QProcess process;
    QStringList arguments;
    arguments << "x" << filePath << "-o" + outputPath << "-y";

    process.start(sevenZipPath, arguments);
    process.waitForFinished(-1);

    if (process.exitCode() != 0) {
        errorMessage = QString::fromUtf8(process.readAllStandardError());
        return false;
    }

    return true;
}

// ============================================================
// دالة عرض المحتويات
// ============================================================
QStringList getSystemArchiveContents(const QString &filePath, QString &errorMessage)
{
    errorMessage.clear();
    QStringList files;

    if (!QFileInfo(filePath).exists()) {
        errorMessage = "The File Does Not Exist: " + filePath;
        return files;
    }

    // ✅ البحث عن 7z
    QString sevenZipPath = find7zExecutable();
    if (sevenZipPath.isEmpty()) {
        errorMessage = "7z executable not found. Please install p7zip-full.";
        return files;
    }

    QProcess process;
    process.start(sevenZipPath, {"l", filePath});
    process.waitForFinished(-1);

    QString output = QString::fromUtf8(process.readAllStandardOutput());
    QString error = QString::fromUtf8(process.readAllStandardError());

    qDebug() << "7z stdout: " << output;
    qDebug() << "7z stderr: " << error;

    if (process.exitCode() != 0) {
        errorMessage = "Error to Read Content of File!";
        return files;
    }

    QStringList lines = output.split("\n");
    int separatorCount = 0;

    for (const QString &line : lines) {
        if (line.contains("-----")) {
            separatorCount++;
            continue;
        }
        if (separatorCount != 1) continue;
        if (line.trimmed().isEmpty()) continue;

        QStringList parts = line.trimmed().split(" ", Qt::SkipEmptyParts);
        if (parts.size() >= 1) {
            QString fileName = parts.last();
            if (!fileName.isEmpty() && !fileName.contains("files")) {
                files << fileName;
            }
        }
    }

    return files;
}
