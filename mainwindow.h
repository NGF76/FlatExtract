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

    // ─── دوال عامة للوصول إلى العناصر ───
    QStatusBar* getStatusBar() const;
    QProgressBar* getProgressBar() const;

    // ─── دالة عامة لإنشاء الكائن (اختياري) ───
    static MainWindow* createInstance(QWidget *parent = nullptr) {
        return new MainWindow(parent);
    }

private slots:
    // ─── فتحات الأزرار ───
    void onChooseFileClicked();
    void onChooseDestinationClicked();
    void onExtractClicked();

    // ─── فتحات إضافية ───
    void toggleNightMode();
    void appendDebugMessage(const QString &msg);

signals:
    // ─── إشارات Debug ───
    void debugSignal(const QString &msg);

private:
    // ─── عناصر الواجهة ───
    Ui::MainWindow *ui;
    void setupUI();

    // ─── نافذة Debug Console ───
    QDockWidget *debugDock;
    QPlainTextEdit *debugOutput;

    // ─── دوال خاصة ───
    void setupDebugConsole();
    void enableNightMode(bool enable);
    QString find7zExecutable();

    // ─── متغيرات الحالة ───
    bool isNightMode = false;
};

#endif // MAINWINDOW_H
