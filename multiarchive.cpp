// ============================================================
// multiarchive.cpp
// إدارة الملفات المتعددة (ZIP و TAR)
// ============================================================

#include "multiarchive.h"
#include "tarextract.h"
#include "tool.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QListWidget>
#include <QStatusBar>
#include <QDebug>

// ============================================================
// دوال عامة (Public Functions)
// ============================================================

/**
 * @brief تحميل وقراءة ملفات متعددة وعرضها في QListWidget
 * @param listWidget مؤشر إلى QListWidget لعرض الملفات
 * @param statusBar مؤشر إلى QStatusBar لعرض الحالة
 */
void MultiArchive::loadMultipleFiles(QListWidget *listWidget, QStatusBar *statusBar)
{
    // 1. فتح نافذة اختيار الملفات المتعددة
    QStringList filePaths = QFileDialog::getOpenFileNames(
        nullptr,
        "اختر ملفات متعددة",
        "",
        "Archives (*.zip *.tar *.tgz *.tar.gz *.tbz *.tar.bz *.tbz2 *.tar.bz2 *.tar.xz)"
        );

    // 2. إذا لم يختر المستخدم ملفات، أوقف التنفيذ
    if (filePaths.isEmpty()) {
        qDebug() << "⚠️ لم يتم اختيار أي ملف";
        return;
    }

    // 3. تنظيف القائمة وعرض رسالة الحالة
    listWidget->clear();
    statusBar->showMessage("✅ تم اختيار " + QString::number(filePaths.size()) + " ملف", 5000);

    // 4. معالجة كل ملف تم اختياره
    for (const QString &filePath : filePaths) {
        // 4a. عرض اسم الملف
        listWidget->addItem("━━━━━━━━━━━━━━━━━━━━");
        listWidget->addItem("📦 " + QFileInfo(filePath).fileName());
        listWidget->addItem("━━━━━━━━━━━━━━━━━━━━");

        // 4b. قراءة محتويات الملف حسب نوعه
        QString errorMessage;
        QStringList entries;
        if (QFileInfo(filePath).suffix().compare("zip", Qt::CaseInsensitive) == 0) {
            entries = getArchiveContents(filePath, errorMessage);
        } else if (isSupportedTarArchive(filePath)) {
            entries = getTarArchiveContents(filePath, errorMessage);
        } else {
            errorMessage = "Unsupported archive format.";
        }

        // 4c. عرض محتويات الملف في القائمة
        if (!errorMessage.isEmpty()) {
            listWidget->addItem("  Error: " + errorMessage);
        } else {
            for (const QString &entry : entries) {
                listWidget->addItem("  " + entry);
            }
        }

        // 4e. إضافة سطر فاصل بين الملفات
        listWidget->addItem("");
    }
}
