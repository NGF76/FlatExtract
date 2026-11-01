// ============================================================
// settingsdialog.cpp
// نافذة إعدادات التطبيق
// ============================================================

#include "settingsdialog.h"
#include "ui_settingsdialog.h"
#include "mainwindow.h"

#include <QCheckBox>
#include <QSettings>

// ============================================================
// المُنشئ
// ============================================================

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);

    loadSettings();

    // ربط الأزرار
    connect(ui->back, &QPushButton::clicked, this, &SettingsDialog::onBackClicked);
    connect(ui->apply, &QPushButton::clicked, this, &SettingsDialog::onApplyClicked);
    connect(ui->nightmode, &QCheckBox::clicked, this, &SettingsDialog::toggleNightMode);
}

// ============================================================
// المُدمر
// ============================================================

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

// ============================================================
// دوال خاصة
// ============================================================

void SettingsDialog::loadSettings()
{
    QSettings settings("NGF76", "FlatExtract");

    const bool nightMode = settings.value("NightMode", true).toBool();
    ui->nightmode->setChecked(nightMode);
}

void SettingsDialog::saveSettings()
{
    QSettings settings("NGF76", "FlatExtract");
    settings.setValue("NightMode", ui->nightmode->isChecked());
    settings.sync();
}

// ============================================================
// فتحات الأزرار (Slots)
// ============================================================

void SettingsDialog::onApplyClicked()
{
    saveSettings();

    emit settingsApplied();

    accept();
}

void SettingsDialog::onBackClicked()
{
    reject();
}

void SettingsDialog::reject()
{
    loadSettings();                 // استرجاع الإعدادات القديمة
    applyNightModeToMainWindow();   // إلغاء المعاينة المؤقتة
    QDialog::reject();              // إغلاق بدون حفظ
}

// ============================================================
// دوال عامة
// ============================================================

bool SettingsDialog::isNightMode() const
{
    return ui->nightmode->isChecked();
}

// ============================================================
// تبديل الوضع الليلي (داخل النافذة)
// ============================================================

void SettingsDialog::toggleNightMode()
{
    applyNightModeToMainWindow();
}

// ============================================================
// تطبيق الوضع الليلي على MainWindow
// ============================================================

void SettingsDialog::applyNightModeToMainWindow()
{
    MainWindow *mainWindow = qobject_cast<MainWindow*>(parentWidget());
    if (!mainWindow) return;

    mainWindow->enableNightMode(ui->nightmode->isChecked());
}
