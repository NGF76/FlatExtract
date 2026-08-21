// ============================================================
// mainwindow.h
// النافذة الرئيسية للتطبيق
// ============================================================

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

// ============================================================
// 1. Includes (مكتبات Qt)
// ============================================================
#include <QMainWindow>
#include <QStatusBar>
#include <QProgressBar>
#include <QDockWidget>
#include <QPlainTextEdit>

// ============================================================
// 2. Includes (مكتبات المشروع)
// ============================================================
#include "tool.h"
#include "settingsdialog.h"

// ============================================================
// 3. إعلان Ui::MainWindow
// ============================================================
QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

// ============================================================
// 4. كلاس MainWindow
// ============================================================

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    // ─── المُنشئ والمُدمر ───
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    // ─── دوال عامة ───
    QStatusBar* getStatusBar() const;
    QProgressBar* getProgressBar() const;

    // ─── دوال Night Mode (عامة) ───
    void enableNightMode(bool enable);
    void toggleNightMode();

private slots:
    // ─── فتحات الأزرار الرئيسية ───
    void onChooseFileClicked();
    void onChooseDestinationClicked();
    void onExtractClicked();
    void onSupportClicked();

    // ─── فتحات الإعدادات و Debug ───
    void onSettingsClicked();
    void applySettings();
    void appendDebugMessage(const QString &msg);

signals:
    void debugSignal(const QString &msg);

private:
    // ─── عناصر الواجهة ───
    Ui::MainWindow *ui;

    // ─── نافذة Debug Console ───
    QDockWidget *debugDock;
    QPlainTextEdit *debugOutput;

    // ─── دوال خاصة ───
    void setupDebugConsole();
    void setupUI();
    QString find7zExecutable();

    // ─── متغيرات الحالة ───
    bool isNightMode = false;
};

#endif // MAINWINDOW_H
