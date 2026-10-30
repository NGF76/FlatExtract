// ============================================================
// extractthread.cpp
// خيط منفصل لاستخراج الملفات
// ============================================================

#include "extractthread.h"
#include "tool.h"
#include "isotool.h"
#include "xboxog_iso.h"
#include "tarextract.h"
#include "progresshelper.h"
#include <QDebug>
#include <QThread>
#include <QApplication>
#include <QFileInfo>

// ============================================================
// المُنشئ
// ============================================================

ExtractThread::ExtractThread(QObject *parent)
    : QThread(parent)
{
}

// ============================================================
// تمرير البيانات إلى الخيط
// ============================================================

void ExtractThread::setData(const QString &filePath, const QString &outputPath)
{
    m_filePath = filePath;
    m_outputPath = outputPath;
}

// ============================================================
// دالة التشغيل الرئيسية (تُنفذ في الخيط المنفصل)
// ============================================================

void ExtractThread::run()
{
    QString errorMessage;
    bool success = false;

    // تحديد نوع الملف
    QString suffix = QFileInfo(m_filePath).suffix().toLower();
    qDebug() << "🔍 Thread - File suffix:" << suffix;

    auto progressCallback = [this](int current, int total, const QString &) {
        emit progressUpdated(current, total);
    };

    //  اختيار الدالة المناسبة حسب نوع الملف
    if (suffix == "iso") {
        if (isXboxOgIso(m_filePath)) {
            success = extractXboxOgIso(m_filePath, m_outputPath, errorMessage, progressCallback);
        } else {
            success = extractAllFromIso(m_filePath, m_outputPath, errorMessage, progressCallback);
        }
    }
    else if (suffix == "zip") {
        //  استخراج ZIP (مع تقدم)
        success = extractArchiveWithProgress(m_filePath, m_outputPath, errorMessage, progressCallback);
    }
    else if (isSupportedTarArchive(m_filePath)) {
        success = extractTarArchive(m_filePath, m_outputPath, errorMessage, progressCallback);
    }
    else {
        errorMessage = QStringLiteral("Unsupported archive format: ") + m_filePath;
    }

    //  إرسال إشارة النجاح أو الفشل
    emit finished(success, success ? "Extraction completed successfully" : errorMessage);
}
