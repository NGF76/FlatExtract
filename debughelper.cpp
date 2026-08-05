// ============================================================
// debughelper.cpp
// دوال التصحيح (Debug) - تنسيق Terminal
// ============================================================

#include "debughelper.h"
#include <QFileInfo>
#include <QDebug>

// ============================================================
// دوال Debug الأساسية
// ============================================================

void debugFunctionStart(const QString &functionName)
{
    qDebug() << "========================================";
    qDebug() << "[START] " << functionName;
    qDebug() << "========================================";
}

void debugFunctionEnd(const QString &functionName)
{
    qDebug() << "========================================";
    qDebug() << "[END] " << functionName;
    qDebug() << "========================================";
}

void debugVariable(const QString &varName, const QString &value)
{
    qDebug() << "[VAR] " << varName << " = " << value;
}

void debugFileList(const QString &label, const QStringList &files)
{
    qDebug() << "[LIST] " << label << " (" << files.size() << " files):";
    for (int i = 0; i < files.size(); ++i) {
        qDebug() << "   [" << i << "] " << files[i];
    }
}

void debugError(const QString &errorMessage)
{
    qDebug() << "[ERROR] " << errorMessage;
}

// ============================================================
// دوال معلومات الملف
// ============================================================

void debugFileInfo(const QString &filePath)
{
    QFileInfo info(filePath);

    qDebug() << "[FILE] Path: " << filePath;
    qDebug() << "[FILE] Exists: " << (info.exists() ? "Yes" : "No");
    qDebug() << "[FILE] Size: " << info.size() << " bytes";
    qDebug() << "[FILE] Suffix: " << info.suffix();
    qDebug() << "[FILE] Readable: " << (info.isReadable() ? "Yes" : "No");
}

// ============================================================
// دوال خاصة بـ QuaZIP
// ============================================================

void debugQuaZipResult(const QString &operation, bool success, const QString &details)
{
    if (success) {
        qDebug() << "[QUAZIP] " << operation << " SUCCESS"
                 << (details.isEmpty() ? "" : " | " + details);
    } else {
        qDebug() << "[QUAZIP] " << operation << " FAILED"
                 << (details.isEmpty() ? "" : " | " + details);
    }
}

void debugExtractFile(const QString &fileName, bool success)
{
    if (success) {
        qDebug() << "   [OK] " << fileName;
    } else {
        qDebug() << "   [FAIL] " << fileName;
    }
}

void debugQuaZipTestStart()
{
    qDebug() << "========================================";
    qDebug() << "[QUAZIP TEST] START";
    qDebug() << "========================================";
}

void debugQuaZipTestEnd()
{
    qDebug() << "========================================";
    qDebug() << "[QUAZIP TEST] END";
    qDebug() << "========================================";
}
