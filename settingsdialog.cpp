// ============================================================
// settingsdialog.cpp
// نافذة إعدادات التطبيق
// ============================================================

#include "settingsdialog.h"
#include "ui_settingsdialog.h"
#include "mainwindow.h"

#include <QCheckBox>
#include <QComboBox>
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
    connect(ui->darkMode, &QCheckBox::clicked, this, &SettingsDialog::toggleNightMode);
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
    ui->darkMode->setChecked(nightMode);

    const int fontSizeIndex = settings.value("FontSizeIndex", 1).toInt();
    ui->fontSizeCombo->setCurrentIndex(qBound(0, fontSizeIndex, 2));

    const bool debugConsole = settings.value("DebugConsoleOnStartup", false).toBool();
    ui->debugConsoleOnStartup->setChecked(debugConsole);

    const bool rememberDest = settings.value("RememberDestination", true).toBool();
    ui->rememberDestination->setChecked(rememberDest);

    const int overwriteBehavior = settings.value("OverwriteBehavior", 0).toInt();
    ui->overwriteBehaviorCombo->setCurrentIndex(qBound(0, overwriteBehavior, 3));

    const bool createSubfolder = settings.value("CreateSubfolderPerArchive", true).toBool();
    ui->createSubfolderPerArchive->setChecked(createSubfolder);
}

void SettingsDialog::saveSettings()
{
    QSettings settings("NGF76", "FlatExtract");
    settings.setValue("NightMode", ui->darkMode->isChecked());
    settings.setValue("FontSizeIndex", ui->fontSizeCombo->currentIndex());
    settings.setValue("DebugConsoleOnStartup", ui->debugConsoleOnStartup->isChecked());
    settings.setValue("RememberDestination", ui->rememberDestination->isChecked());
    settings.setValue("OverwriteBehavior", ui->overwriteBehaviorCombo->currentIndex());
    settings.setValue("CreateSubfolderPerArchive", ui->createSubfolderPerArchive->isChecked());
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
    return ui->darkMode->isChecked();
}

int SettingsDialog::fontSizeIndex() const
{
    return ui->fontSizeCombo->currentIndex();
}

bool SettingsDialog::showDebugConsoleOnStartup() const
{
    return ui->debugConsoleOnStartup->isChecked();
}

bool SettingsDialog::rememberDestination() const
{
    return ui->rememberDestination->isChecked();
}

int SettingsDialog::overwriteBehavior() const
{
    return ui->overwriteBehaviorCombo->currentIndex();
}

bool SettingsDialog::createSubfolderPerArchive() const
{
    return ui->createSubfolderPerArchive->isChecked();
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

    mainWindow->enableNightMode(ui->darkMode->isChecked());
}
