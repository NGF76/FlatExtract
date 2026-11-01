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
    bool isNightMode() const;

    // ─── تبديل الوضع الليلي (داخل النافذة) ───
    void toggleNightMode();

signals:
    // ─── إشارة عند تطبيق الإعدادات ───
    void settingsApplied();

private slots:
    // ─── فتحات الأزرار ───
    void onApplyClicked();
    void onBackClicked();

protected:
    // ─── إلغاء النافذة (زر Cancel أو Esc) ───
    void reject() override;

private:
    // ─── عناصر الواجهة ───
    Ui::SettingsDialog *ui;

    // ─── دوال خاصة ───
    void loadSettings();
    void saveSettings();

    void applyNightModeToMainWindow();
};

#endif // SETTINGSDIALOG_H
