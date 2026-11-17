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
    int fontSizeIndex() const;               // 0 = Small (10pt), 1 = Medium (12pt), 2 = Large (14pt)
    bool showDebugConsoleOnStartup() const;
    bool rememberDestination() const;
    int overwriteBehavior() const;           // 0 = Ask, 1 = Overwrite, 2 = Skip, 3 = Rename
    bool createSubfolderPerArchive() const;

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
