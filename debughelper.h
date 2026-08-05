// ============================================================
// debughelper.h
// دوال التصحيح (Debug) لتتبع البرنامج
// ============================================================

#ifndef DEBUGHELPER_H
#define DEBUGHELPER_H

#include <QString>
#include <QStringList>
#include <QDebug>

// ============================================================
// دوال Debug الأساسية
// ============================================================

/**
 * @brief طباعة رسالة بداية دالة
 * @param functionName اسم الدالة
 */
void debugFunctionStart(const QString &functionName);

/**
 * @brief طباعة رسالة نهاية دالة
 * @param functionName اسم الدالة
 */
void debugFunctionEnd(const QString &functionName);

/**
 * @brief طباعة قيمة متغير
 * @param varName اسم المتغير
 * @param value قيمة المتغير
 */
void debugVariable(const QString &varName, const QString &value);

/**
 * @brief طباعة قائمة ملفات
 * @param label عنوان القائمة
 * @param files قائمة الملفات
 */
void debugFileList(const QString &label, const QStringList &files);

/**
 * @brief طباعة رسالة خطأ
 * @param errorMessage نص الخطأ
 */
void debugError(const QString &errorMessage);

/**
 * @brief طباعة معلومات ملف
 * @param filePath مسار الملف
 */
void debugFileInfo(const QString &filePath);

// ============================================================
// دوال خاصة بـ QuaZIP
// ============================================================

/**
 * @brief طباعة نتيجة عملية QuaZIP
 * @param operation اسم العملية (مثل "extractDir", "getFileList")
 * @param success هل نجحت العملية؟
 * @param details تفاصيل إضافية (مثل عدد الملفات)
 */
void debugQuaZipResult(const QString &operation, bool success, const QString &details = "");

/**
 * @brief طباعة تفاصيل استخراج ملف
 * @param fileName اسم الملف
 * @param success هل تم استخراجه بنجاح؟
 */
void debugExtractFile(const QString &fileName, bool success);

/**
 * @brief طباعة بداية اختبار QuaZIP
 */
void debugQuaZipTestStart();

/**
 * @brief طباعة نهاية اختبار QuaZIP
 */
void debugQuaZipTestEnd();

#endif // DEBUGHELPER_H
