// ============================================================
// multiarchive.cpp
// إدارة الملفات المتعددة (ZIP, RAR, 7z)
// ============================================================

#include "multiarchive.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QListWidget>
#include <QStatusBar>
#include <QProcess>
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
        "Archives (*.zip *.rar *.7z)"
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

        // 4b. محاولة قراءة محتويات الملف باستخدام 7z
        QStringList arguments;
        arguments << "l" << filePath;

        QProcess process;
        process.start("7z", arguments);

        if (process.waitForFinished(5000)) {
            QString output = process.readAllStandardOutput();
            QStringList lines = output.split('\n');

            // 4c. عرض محتويات الملف في القائمة
            for (const QString &line : lines) {
                if (!line.trimmed().isEmpty()) {
                    listWidget->addItem("  " + line);
                }
            }
        } else {
            // 4d. إذا فشلت القراءة، عرض رسالة خطأ
            listWidget->addItem("  ⚠️ فشل في القراءة (انتهت المهلة)");
        }

        // 4e. إضافة سطر فاصل بين الملفات
        listWidget->addItem("");
    }
}
