// ============================================================
// progresshelper.h
// دوال التقدم (Progress Bar) والحالة (Status Bar)
// ============================================================

#ifndef PROGRESSHELPER_H
#define PROGRESSHELPER_H

#include <QString>

// ============================================================
// إعلانات مسبقة (Forward Declarations)
// ============================================================

class QProgressBar;
class QStatusBar;

// ============================================================
// دوال Status Bar (شريط الحالة)
// ============================================================

/**
 * @brief عرض رسالة في Status Bar
 * @param statusBar شريط الحالة
 * @param message النص المراد عرضه
 * @param timeout مدة العرض بالمللي ثانية (0 = دائم)
 */
void showStatusMessage(QStatusBar *statusBar, const QString &message, int timeout = 0);

/**
 * @brief عرض رسالة بداية الاستخراج
 * @param statusBar شريط الحالة
 */
void showExtractStart(QStatusBar *statusBar);

/**
 * @brief عرض رسالة نهاية الاستخراج
 * @param statusBar شريط الحالة
 * @param count عدد الملفات المستخرجة
 */
void showExtractEnd(QStatusBar *statusBar, int count);

/**
 * @brief تحديث حالة الاستخراج أثناء العملية
 * @param statusBar شريط الحالة
 * @param current اسم الملف الحالي
 * @param index رقم الملف الحالي
 * @param total العدد الإجمالي للملفات
 */
void updateExtractStatus(QStatusBar *statusBar, const QString &current, int index, int total);

// ============================================================
// دوال Progress Bar (شريط التقدم)
// ============================================================

/**
 * @brief تهيئة شريط التقدم
 * @param progressBar شريط التقدم
 * @param max القيمة القصوى (عدد الملفات)
 */
void initProgressBar(QProgressBar *progressBar, int max);

/**
 * @brief تحديث قيمة شريط التقدم
 * @param progressBar شريط التقدم
 * @param value القيمة الحالية
 */
void setProgressValue(QProgressBar *progressBar, int value);

/**
 * @brief إعادة ضبط شريط التقدم (إخفاؤه)
 * @param progressBar شريط التقدم
 */
void resetProgressBar(QProgressBar *progressBar);

/**
 * @brief إظهار أو إخفاء شريط التقدم
 * @param progressBar شريط التقدم
 * @param visible true = إظهار، false = إخفاء
 */
void setProgressBarVisible(QProgressBar *progressBar, bool visible);

// ============================================================
// دوال مساعدة (Helper Functions)
// ============================================================

/**
 * @brief تحديث شريط التقدم بقيمة ونطاق محددين
 * @param progressBar شريط التقدم
 * @param value القيمة الحالية
 * @param max القيمة القصوى
 */
void updateProgressBar(QProgressBar *progressBar, int value, int max);

#endif // PROGRESSHELPER_H
