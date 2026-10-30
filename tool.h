// ============================================================
// tool.h
// تعريفات دوال التعامل مع الملفات المضغوطة (QuaZIP)
// ============================================================

#ifndef TOOL_H
#define TOOL_H

#include <QString>
#include <QStringList>

#include "extractprogress.h"

// ============================================================
// دوال عرض المحتويات
// ============================================================

/**
 * @brief عرض محتويات الملف المضغوط
 * @param filePath مسار الملف المضغوط
 * @param errorMessage رسالة الخطأ (للتمرير)
 * @return قائمة بأسماء الملفات داخل الأرشيف
 */
QStringList getArchiveContents(const QString &filePath, QString &errorMessage);

// ============================================================
// دوال الاستخراج
// ============================================================

/**
 * @brief استخراج الملفات من الأرشيف (بدون تقدم)
 * @param filePath مسار الملف المضغوط
 * @param outputPath مجلد الوجهة
 * @param errorMessage رسالة الخطأ (للتمرير)
 * @return true إذا نجحت العملية، false إذا فشلت
 */
bool extractArchive(const QString &filePath, const QString &outputPath, QString &errorMessage);

/**
 * @brief استخراج الملفات مع تحديث شريط التقدم وحالة التطبيق
 * @param filePath مسار الملف المضغوط
 * @param outputPath مجلد الوجهة
 * @param errorMessage رسالة الخطأ (للتمرير)
 * @param progressCallback دالة تُستدعى بعد استخراج كل ملف
 * @return true إذا نجحت العملية، false إذا فشلت
 */
bool extractArchiveWithProgress(const QString &filePath, const QString &outputPath, QString &errorMessage,
                                ExtractProgressCallback progressCallback = nullptr);

#endif // TOOL_H
