#include "multiarchive.h"
#include <QFileDialog>
#include <QFileInfo>
#include <QListWidget>
#include <QStatusBar>
#include <QProcess>


void MultiArchive::loadMultipleFiles(QListWidget *listWidget, QStatusBar *statusBar)
{
    QStringList filePaths = QFileDialog::getOpenFileNames(
        nullptr,
        "اختر ملفات متعددة",
        "",
        "Archives (*.zip *.rar *.7z)"
        );

    if (filePaths.isEmpty()) {
        return;
    }

    listWidget->clear();
    statusBar->showMessage("✅ تم اختيار " + QString::number(filePaths.size()) + " ملف", 5000);

    for (const QString &filePath : filePaths) {
        listWidget->addItem("━━━━━━━━━━━━━━━━━━━━");
        listWidget->addItem("📦 " + QFileInfo(filePath).fileName());
        listWidget->addItem("━━━━━━━━━━━━━━━━━━━━");

        // مثال: عرض المحتويات باستخدام 7z
        QStringList arguments;
        arguments << "l" << filePath;

        QProcess process;
        process.start("7z", arguments);

        if (process.waitForFinished(5000)) {
            QString output = process.readAllStandardOutput();
            QStringList lines = output.split('\n');

            for (const QString &line : lines) {
                if (!line.trimmed().isEmpty()) {
                    listWidget->addItem("  " + line);
                }
            }
        } else {
            listWidget->addItem("  ⚠️ فشل في القراءة");
        }

        listWidget->addItem("");
    }
}
