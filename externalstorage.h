// ============================================================
// externalstorage.h
// اكتشاف وسائط التخزين الخارجية (USB، بطاقات SD، أقراص خارجية)
// ============================================================

#ifndef EXTERNALSTORAGE_H
#define EXTERNALSTORAGE_H

#include <QList>
#include <QString>

struct ExternalStorageVolume
{
    QString displayName;   // اسم الوحدة (التسمية أو اسم مجلد التحميل)
    QString rootPath;      // مسار نقطة التحميل
    QString device;        // مثال: /dev/sdb1
    QString fileSystem;    // مثال: fuseblk
    qint64 bytesTotal = 0;
    qint64 bytesAvailable = 0;
    bool readOnly = false;
};

// إرجاع وسائط التخزين الخارجية المثبتة حالياً والجاهزة للاستخدام.
QList<ExternalStorageVolume> externalStorageVolumes();

// إرجاع أقرب مجلد موجود للمسار المطلوب (أو المجلد الرئيسي للمستخدم).
QString nearestExistingDirectory(const QString &path);

#endif // EXTERNALSTORAGE_H
