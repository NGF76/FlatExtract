// ============================================================
// settingsdialog.h
// نافذة إعدادات التطبيق
// ============================================================

#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>

// ============================================================
// إعلان مسبق لـ Ui::SettingsDialog
// ============================================================

namespace Ui {
class SettingsDialog;
}

// ============================================================
// كلاس SettingsDialog
// ============================================================

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    // ─── المُنشئ والمُدمر ───
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog();

    // ─── دوال عامة للوصول إلى الإعدادات ───
    bool isDebugConsole() const;
    bool isNightMode() const;

    // ─── تبديل الوضع الليلي (داخل النافذة) ───
    void toggleNightMode();

signals:
    // ─── إشارة عند تطبيق الإعدادات ───
    void settingsApplied();

    // ─── إشارة عند تغيير Night Mode ───
    void nightModeToggled(bool enabled);

private slots:
    // ─── فتحات الأزرار ───
    void onApplyClicked();
    void onBackClicked();

private:
    // ─── عناصر الواجهة ───
    Ui::SettingsDialog *ui;

    // ─── دوال خاصة ───
    void loadSettings();
    void saveSettings();

    void applyNightModeToMainWindow();

    // ─── متغيرات الحالة ───
    bool isNightModeEnabled = false;
};

#endif // SETTINGSDIALOG_H
