#ifndef ISOTOOL_H
#define ISOTOOL_H

// ============================================================
// isotool.h
// دوال التعامل مع ملفات ISO باستخدام libcdio
// ============================================================


#include <QString>
#include <QStringList>
#include <functional>


/**
 * @brief فتح ملف ISO وقراءة محتوياته
 * @param isoPath مسار ملف ISO
 * @param errorMessage رسالة الخطأ (للتمرير)
 * @return قائمة بأسماء الملفات داخل الـ ISO
 */

QStringList getIsoContents(const QString &isoPath, QString &errorMessage);

/**
 * @brief استخراج ملف من ISO إلى مجلد محدد
 * @param isoPath مسار ملف ISO
 * @param fileName اسم الملف داخل الـ ISO
 * @param outputPath مجلد الوجهة
 * @param errorMessage رسالة الخطأ (للتمرير)
 * @return true إذا نجحت العملية، false إذا فشلت
 */

// Callback signature: (current index, total count, current file name)
using ExtractProgressCallback = std::function<void(int, int, const QString&)>;


bool extractFileFromIso(const QString &isoPath, const QString &outputPath ,QString &errorMessage);
bool extractAllFromIso(const QString &isoPath, const QString &outputPath ,QString &errorMessage);
//bool extractAllFromIso(const QString &isoPath, const QString &outputPath,
                       //QString &errorMessage,                                disbled
                       //ExtractProgressCallback progressCallback = nullptr);
//const QString &fileName

/**
 * @brief اختبار وجود مكتبة libcdio
 * @return true إذا كانت المكتبة موجودة، false إذا لم تكن
 */


bool testLibCdio();

#endif // ISOTOOL_H
