// ============================================================
// threadhub.cpp
// خيط منفصل لاختيار الملفات والمجلدات
// ============================================================

#include "threadhub.h"
#include <QFileDialog>
#include <QDir>

ThreadHub::ThreadHub(QObject *parent)
    : QObject(parent)
{
}

// ─── اختيار ملف ─────────────────────────────────────────────

void ThreadHub::chooseFile()
{
    QString filePath = QFileDialog::getOpenFileName(
        nullptr,
        "اختر ملف مضغوط أو ISO",
        QDir::homePath(),
        "All Supported (*.zip *.tar *.tgz *.tar.gz *.tbz *.tar.bz *.tbz2 *.tar.bz2 *.tar.xz *.iso);;"
        "ZIP Files (*.zip);;"
        "TAR Files (*.tar *.tgz *.tar.gz *.tbz *.tar.bz *.tbz2 *.tar.bz2 *.tar.xz);;"
        "ISO Files (*.iso);;All Files (*)"
        );

    emit fileSelected(filePath);
    emit finished();
}

// ─── اختيار مجلد ─────────────────────────────────────────────

void ThreadHub::chooseDirectory()
{
    QString folderPath = QFileDialog::getExistingDirectory(
        nullptr,
        "اختر مجلد الوجهة",
        QDir::homePath(),
        QFileDialog::ShowDirsOnly
        );

    emit directorySelected(folderPath);
    emit finished();
}
