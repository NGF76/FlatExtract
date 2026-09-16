// ============================================================
// progresshelper.cpp
// دوال التقدم (Progress Bar) والحالة (Status Bar)
// ============================================================

#include "progresshelper.h"
#include <QProgressBar>
#include <QStatusBar>
#include <QApplication>
#include <QDebug>

// ============================================================
// دوال Status Bar (شريط الحالة)
// ============================================================

void showStatusMessage(QStatusBar *statusBar, const QString &message, int timeout)
{
    if (!statusBar) return;
    statusBar->showMessage(message, timeout);
}

void showExtractStart(QStatusBar *statusBar)
{
    showStatusMessage(statusBar, "⏳ Extracting...", 0);
}

void showExtractEnd(QStatusBar *statusBar, int count)
{
    QString msg = QString(" %1 %2 successfully extracted.")
                      .arg(count)
                      .arg(count == 1 ? "File" : "Files/Folders");
    showStatusMessage(statusBar, msg, 7000);
}

void updateExtractStatus(QStatusBar *statusBar, const QString &current, int index, int total)
{
    if (!statusBar) return;

    QString msg = QString(" Extraction %1 (%2/%3)")
                      .arg(current)
                      .arg(index)
                      .arg(total);

    statusBar->showMessage(msg, 0);
    QApplication::processEvents();
}

// ============================================================
// دوال Progress Bar (شريط التقدم)
// ============================================================

void initProgressBar(QProgressBar *progressBar, int max)
{
    if (!progressBar) return;

    progressBar->setRange(0, max);
    progressBar->setValue(0);
    progressBar->setVisible(true);
    QApplication::processEvents();
}

void setProgressValue(QProgressBar *progressBar, int value)
{
    if (!progressBar) return;

    progressBar->setValue(value);
    QApplication::processEvents();
}

void resetProgressBar(QProgressBar *progressBar)
{
    if (!progressBar) return;

    qDebug() << "[DEBUG] resetProgressBar called!";
    progressBar->setValue(0);
    progressBar->setVisible(false);
    QApplication::processEvents();
}

void setProgressBarVisible(QProgressBar *progressBar, bool visible)
{
    if (!progressBar) return;

    progressBar->setVisible(visible);
    QApplication::processEvents();
}

// ============================================================
// دالة مساعدة (تحديث شامل)
// ============================================================

void updateProgressBar(QProgressBar *progressBar, int value, int max)
{
    if (!progressBar) return;

    if (max <= 0) {
        progressBar->setRange(0, 0);  // وضع غير محدد (Indeterminate)
        return;
    }

    progressBar->setRange(0, max);
    progressBar->setValue(value);
    QApplication::processEvents();
}
