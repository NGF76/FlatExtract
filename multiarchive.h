// ============================================================
// multiarchive.h
// إدارة الملفات المضغوطة المتعددة (Multi-Archive)
// ============================================================

#ifndef MULTIARCHIVE_H
#define MULTIARCHIVE_H

#include <QStringList>

// ============================================================
// إعلانات مسبقة (Forward Declarations)
// ============================================================

class QListWidget;
class QStatusBar;

// ============================================================
// كلاس MultiArchive
// ============================================================

/**
 * @brief كلاس مساعد للتعامل مع ملفات مضغوطة متعددة
 */
class MultiArchive
{
public:
    /**
     * @brief تحميل ملفات متعددة وعرضها في QListWidget
     * @param listWidget مؤشر إلى QListWidget لعرض الملفات
     * @param statusBar مؤشر إلى QStatusBar لعرض الحالة
     */
    static void loadMultipleFiles(QListWidget *listWidget, QStatusBar *statusBar);
};

#endif // MULTIARCHIVE_H
