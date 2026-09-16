// ============================================================
// settingsdialog.cpp
// نافذة إعدادات التطبيق
// ============================================================

#include "settingsdialog.h"
#include "ui_settingsdialog.h"
#include "mainwindow.h"
#include <QSettings>
#include <QMainWindow>
#include <QDebug>

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
    connect(ui->nightmode, &QPushButton::clicked, this, &SettingsDialog::toggleNightMode);

    // تحديث نص الزر عند الضغط
    connect(ui->nightmode, &QPushButton::clicked, this, [this]() {
        bool checked = ui->nightmode->isChecked();
        ui->nightmode->setText(checked ? "  Light Mode" : "  Night Mode");
    });

    setFixedSize(500, 300);
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

    bool nightMode = settings.value("NightMode", false).toBool();
    ui->nightmode->setChecked(nightMode);
    ui->nightmode->setText(nightMode ? " Light Mode" : " Night Mode");

    // (تم تعطيل Debug Console مؤقتاً)
}

void SettingsDialog::saveSettings()
{
    QSettings settings("NGF76", "FlatExtract");
    settings.setValue("NightMode", ui->nightmode->isChecked());
}

// ============================================================
// فتحات الأزرار (Slots)
// ============================================================

void SettingsDialog::onApplyClicked()
{
    qDebug() << " [1] onApplyClicked called!";
    qDebug() << " NightMode checked:" << ui->nightmode->isChecked();

    saveSettings();
    applyNightModeToMainWindow();

    emit nightModeToggled(ui->nightmode->isChecked());
    emit settingsApplied();
    accept();
}

void SettingsDialog::onBackClicked()
{
    loadSettings();  // استرجاع الإعدادات القديمة
    reject();        // إغلاق بدون حفظ
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
    isNightModeEnabled = !isNightModeEnabled;

    // تطبيق الوضع الليلي على النافذة الرئيسية
    applyNightModeToMainWindow();

    // تحديث نص الزر
    ui->nightmode->setText(isNightModeEnabled ? "  Light Mode" : "  Night Mode");
}

// ============================================================
// تطبيق الوضع الليلي على MainWindow
// ============================================================

void SettingsDialog::applyNightModeToMainWindow()
{
    MainWindow *mainWindow = qobject_cast<MainWindow*>(parentWidget());
    if (!mainWindow) return;

    bool isNightMode = ui->nightmode->isChecked();

    if (isNightMode) {
        mainWindow->setStyleSheet(
            "QMainWindow { background-color: #1e1e1e; }"
            "QWidget { background-color: #1e1e1e; color: #d4d4d4; }"
            "QPushButton { background-color: #3c3c3c; color: #d4d4d4; border: 1px solid #555; border-radius: 5px; }"
            "QPushButton:hover { background-color: #505050; }"
            "QLineEdit { background-color: #2d2d2d; color: #d4d4d4; border: 1px solid #555; border-radius: 5px; padding: 5px; }"
            "QListWidget { background-color: #2d2d2d; color: #d4d4d4; border: 1px solid #555; }"
            "QListWidget::item:selected { background-color: #3c3c3c; }"
            "QLabel { color: #d4d4d4; }"
            "QStatusBar { background-color: #2d2d2d; color: #d4d4d4; }"
            );
    } else {
        mainWindow->setStyleSheet("");
    }
}
