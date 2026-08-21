// ============================================================
// extractthread.cpp
// خيط منفصل لاستخراج الملفات
// ============================================================

#include "extractthread.h"
#include "tool.h"
#include "isotool.h"
#include "progresshelper.h"
#include <QDebug>
#include <QThread>
#include <QApplication>
#include <QFileInfo>
#include <QTypeInfo>

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

    // ✅ استدعاء الدالة المناسبة حسب نوع الملف
    if (suffix == "iso") {
        // ✅ استخراج ISO (مع تحديث التقدم)
        success = extractAllFromIso(m_filePath, m_outputPath, errorMessage);
    } else {
        // ✅ استخراج الملفات المضغوطة (مع تقدم)
        success = extractArchiveWithProgress(m_filePath, m_outputPath, errorMessage, nullptr);
    }

    // ✅ إرسال إشارة النجاح أو الفشل
    emit finished(success, success ? "تم الاستخراج بنجاح" : errorMessage);
}
