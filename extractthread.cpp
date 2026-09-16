// ============================================================
// extractthread.cpp
// خيط منفصل لاستخراج الملفات
// ============================================================

#include "extractthread.h"
#include "tool.h"
#include "isotool.h"
#include "Systemextract.h"
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

    //  التعامل مع الصيغ المركبة (tar.gz, tar.bz2, tar.xz)
    if (m_filePath.endsWith(".tar.gz", Qt::CaseInsensitive) ||
        m_filePath.endsWith(".tar.bz2", Qt::CaseInsensitive) ||
        m_filePath.endsWith(".tar.xz", Qt::CaseInsensitive)) {
        suffix = "tar";
    }

    //  اختيار الدالة المناسبة حسب نوع الملف
    if (suffix == "iso") {
        //  استخراج ISO (مع تحديث التقدم)
        success = extractAllFromIso(m_filePath, m_outputPath, errorMessage);
    }
    else if (suffix == "zip") {
        //  استخراج ZIP (مع تقدم)
        success = extractArchiveWithProgress(m_filePath, m_outputPath, errorMessage, nullptr);
    }
    else {
        //  استخراج الصيغ الأخرى باستخدام 7z: 7z, tar, gz, bz2, xz, rar
        success = extractWithSystemTool(m_filePath, m_outputPath, errorMessage);
    }

    //  إرسال إشارة النجاح أو الفشل
    emit finished(success, success ? "Extraction completed successfully" : errorMessage);
}
