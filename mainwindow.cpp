// ============================================================
// mainwindow.cpp
// النافذة الرئيسية للتطبيق
// ============================================================

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "tool.h"
#include "debughelper.h"
#include "progresshelper.h"
#include "isotool.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QMainWindow>
#include <QDir>
#include <QFileInfo>
#include <QDirIterator>
#include <QStatusBar>
#include <QDebug>
#include <cdio/cdio.h>

// ============================================================
// دالة معالجة رسائل Debug (عامة)
// ============================================================

static void debugMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(type);
    Q_UNUSED(context);

    fprintf(stderr, "%s\n", msg.toLocal8Bit().constData());

    MainWindow *mainWindow = qobject_cast<MainWindow*>(qApp->activeWindow());
    if (mainWindow) {
        emit mainWindow->debugSignal(msg);
    }
}

// ============================================================
// المُنشئ (Constructor)
// ============================================================

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    // إعداد نافذة Debug Console
    setupDebugConsole();

    // ربط الأزرار
    connect(ui->fileselection, &QPushButton::clicked, this, &MainWindow::onChooseFileClicked);
    connect(ui->chooseadestination, &QPushButton::clicked, this, &MainWindow::onChooseDestinationClicked);
    connect(ui->startextract, &QPushButton::clicked, this, &MainWindow::onExtractClicked);
    connect(ui->nightmode, &QPushButton::clicked, this, &MainWindow::toggleNightMode);

    // ربط زر Debug Console
    connect(ui->debugconsole, &QPushButton::clicked, this, [=]() {
        debugDock->setVisible(!debugDock->isVisible());
    });

    // ربط إشارة Debug
    connect(this, &MainWindow::debugSignal, this, &MainWindow::appendDebugMessage);

    // تعيين مجلد افتراضي للوجهة
    ui->destinationLineEdit->setText(QDir::homePath() + "/extracted");

    // تعيين حجم النافذة وعنوانها
    setFixedSize(900, 600);
    setWindowTitle("FlatExtract");

    // تطبيق التصميم
     setupUI();
}
void MainWindow::setupDebugConsole()
{
    // إعداد نافذة Debug Console
    debugDock = new QDockWidget("Debug Console", this);
    debugDock->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);

    debugOutput = new QPlainTextEdit(this);
    debugOutput->setReadOnly(true);
    debugOutput->setFont(QFont("Monospace", 10));
    debugOutput->setStyleSheet("background-color: #1e1e1e; color: #d4d4d4;");

    debugDock->setWidget(debugOutput);
    addDockWidget(Qt::BottomDockWidgetArea, debugDock);

    debugDock->setVisible(false);

    // ربط إشارة Debug
    connect(this, &MainWindow::debugSignal, this, &MainWindow::appendDebugMessage);

    // تثبيت معالج Debug (إذا كنت تستخدمه)
    // qInstallMessageHandler(debugMessageHandler);
}

// ============================================================
// المُدمر (Destructor)
// ============================================================

MainWindow::~MainWindow()
{
    delete ui;
}

// ============================================================
// دوال عامة (Public Functions)
// ============================================================

QStatusBar* MainWindow::getStatusBar() const
{
    return ui->statusbar;
}

QProgressBar* MainWindow::getProgressBar() const
{
    return ui->progressBar;
}

// ============================================================
// فتحات (Slots) للأزرار
// ============================================================

// ─── اختيار الملف ─────────────────────────────────────────────

void MainWindow::onChooseFileClicked()
{
    debugFunctionStart("onChooseFileClicked");

    // 1. اختيار الملف
    QString filePath = QFileDialog::getOpenFileName(
        this,
        "اختر ملف مضغوط أو ISO",
        QDir::homePath(),
        "All Supported (*.zip *.7z *.tar.gz *.tar.bz2 *.tar.xz *.iso);;ZIP Files (*.zip);;ISO Files (*.iso);;All Files (*)"
        );

    if (filePath.isEmpty()) {
        debugError("لم يتم اختيار ملف");
        debugFunctionEnd("onChooseFileClicked");
        return;
    }

    ui->filepathlineEdit->setText(filePath);
    debugVariable("File Path", filePath);
    debugFileInfo(filePath);

    // 2. تحديد نوع الملف
    QString suffix = QFileInfo(filePath).suffix().toLower();
    qDebug() << "🔍 File suffix:" << suffix;

    QStringList files;
    QString errorMessage;

    // 3. استدعاء الدالة المناسبة حسب النوع
    if (suffix == "iso") {
        qDebug() << "📀 ISO file detected, using getIsoContents...";
        files = getIsoContents(filePath, errorMessage);
        if (!errorMessage.isEmpty()) {
            QMessageBox::warning(this, "خطأ", errorMessage);
            debugFunctionEnd("onChooseFileClicked");
            return;
        }
    } else {
        qDebug() << "📦 Archive file detected, using getArchiveContents...";
        files = getArchiveContents(filePath, errorMessage);
        if (!errorMessage.isEmpty()) {
            QMessageBox::warning(this, "خطأ", errorMessage);
            debugFunctionEnd("onChooseFileClicked");
            return;
        }
    }

    // 4. عرض النتائج في listWidgetBefore
    ui->listWidgetBefore->clear();
    ui->listWidgetBefore->addItem("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    ui->listWidgetBefore->addItem("📁 " + QFileInfo(filePath).fileName());
    ui->listWidgetBefore->addItem("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");

    if (files.isEmpty()) {
        ui->listWidgetBefore->addItem("⚠️ لا توجد ملفات");
    } else {
        ui->listWidgetBefore->addItem("📊 عدد الملفات: " + QString::number(files.size()));
        ui->listWidgetBefore->addItem("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
        for (const QString &file : files) {
            ui->listWidgetBefore->addItem("📄 " + file);
        }
    }

    // 5. تحديث StatusBar
    showStatusMessage(ui->statusbar, " تم تحميل " + QString::number(files.size()) + " ملف", 5000);

    debugFileList("Files in archive", files);
    debugFunctionEnd("onChooseFileClicked");
}

// ─── اختيار مجلد الوجهة ──────────────────────────────────────

void MainWindow::onChooseDestinationClicked()
{
    debugFunctionStart("onChooseDestinationClicked");

    QString folderPath = QFileDialog::getExistingDirectory(
        this,
        "Choose a Destination",
        QDir::homePath(),
        QFileDialog::ShowDirsOnly
        );

    if (!folderPath.isEmpty()) {
        ui->destinationLineEdit->setText(folderPath);
        ui->listWidgetAfter->clear();
        ui->listWidgetAfter->addItem("📁 " + folderPath + " جاهز للاستخراج");
        debugVariable("Destination Folder", folderPath);
    }

    debugFunctionEnd("onChooseDestinationClicked");
}

// ─── استخراج الملفات ─────────────────────────────────────────

void MainWindow::onExtractClicked()
{
    debugFunctionStart("onExtractClicked");

    QString filePath = ui->filepathlineEdit->text();
    if (filePath.isEmpty()) {
        debugError("لم يتم اختيار ملف");
        QMessageBox::warning(this, "خطأ", "الرجاء اختيار ملف مضغوط أولاً");
        debugFunctionEnd("onExtractClicked");
        return;
    }

    QString outputPath = ui->destinationLineEdit->text();
    if (outputPath.isEmpty()) {
        QDir outputDir = QFileInfo(filePath).absoluteDir();
        outputPath = outputDir.absolutePath() + "/extracted";
    }
    debugVariable("Output Path", outputPath);

    QString suffix = QFileInfo(filePath).suffix().toLower();
    qDebug() << "🔍 Extract - File suffix:" << suffix;

    QString errorMessage;
    bool success = false;

    if (suffix == "iso") {
        qDebug() << "📀 Extracting ISO file...";
        success = extractAllFromIso(filePath, outputPath, errorMessage);
    } else {
        success = extractArchiveWithProgress(filePath, outputPath, errorMessage, this);
    }

    if (!success) {
        ui->statusbar->showMessage("❌ فشل الاستخراج: " + errorMessage, 5000);
        debugError(errorMessage);
        QMessageBox::critical(this, "خطأ", errorMessage);
        debugFunctionEnd("onExtractClicked");
        return;
    }

    // عرض الملفات المستخرجة
    QDir extractedDir(outputPath);
    QStringList files = extractedDir.entryList(QDir::Files | QDir::NoDotAndDotDot);

    if (files.isEmpty()) {
        QDirIterator it(outputPath, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            it.next();
            files << it.fileInfo().absoluteFilePath();
        }
    }

    debugFileList("Extracted files", files);

    ui->listWidgetAfter->clear();
    ui->listWidgetAfter->addItem("📁 " + outputPath + " (تمت العملية)");

    if (files.isEmpty()) {
        ui->listWidgetAfter->addItem("⚠️ لا توجد ملفات مستخرجة");
    } else {
        for (const QString &file : files) {
            ui->listWidgetAfter->addItem("📄 " + file + " (تمت العملية)");
        }
    }

    ui->statusbar->showMessage("✅ تم استخراج " + QString::number(files.size()) + " ملف/مجلد بنجاح", 7000);
    QMessageBox::information(this, "نجاح", "تم استخراج الملفات بنجاح إلى:\n" + outputPath);

    debugFunctionEnd("onExtractClicked");
}

void MainWindow::appendDebugMessage(const QString &msg)
{
    if (debugOutput) {
        debugOutput->appendPlainText(msg);
    }
}

// ─── البحث عن 7z ─────────────────────────────────────────────

QString MainWindow::find7zExecutable()
{
    QStringList possibleNames = {"7z", "7za", "7zr", "gz"};
    QStringList searchPaths = {"/usr/bin/", "/usr/local/bin/"};

    for (const QString &name : possibleNames) {
        for (const QString &path : searchPaths) {
            QFileInfo file(path + name);
            if (file.exists() && file.isExecutable()) {
                return file.absoluteFilePath();
            }
        }
    }
    return "";
}

// ─── تصميم الواجهة (UI Styling) ─────────────────────────────

void MainWindow::setupUI()
{
    // زر الاستخراج (أخضر بتدرج)
    ui->startextract->setStyleSheet(
        "QPushButton {"
        "   background: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1,"
        "       stop:0 rgba(76, 175, 80, 200),"
        "       stop:0.3 rgba(139, 195, 74, 180),"
        "       stop:0.7 rgba(56, 142, 60, 180),"
        "       stop:1 rgba(27, 94, 32, 160));"
        "   color: white;"
        "   border: 1px solid #1B5E20;"
        "   font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "   background: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1,"
        "       stop:0 rgba(102, 187, 106, 220),"
        "       stop:0.3 rgba(165, 214, 167, 200),"
        "       stop:0.7 rgba(76, 175, 80, 200),"
        "       stop:1 rgba(46, 125, 50, 180));"
        "}"
        );

    // زر اختيار الملف (أزرق)
    ui->fileselection->setStyleSheet(
        "QPushButton { background-color: #3498db; color: white; }"
        "QPushButton:hover { background-color: #2980b9; }"
        );

    // زر اختيار الوجهة (أزرق)
    ui->chooseadestination->setStyleSheet(
        "QPushButton { background-color: #3498db; color: white; }"
        "QPushButton:hover { background-color: #2980b9; }"
        );

    // زر Debug Console (أحمر)
    ui->debugconsole->setStyleSheet(
        "QPushButton { background-color: #FF0000; color: white; padding: 6px; }"
        "QPushButton:hover { background-color: #cc0000; }"
        );
}


void MainWindow::toggleNightMode()
{
    isNightMode = !isNightMode;

    if (isNightMode) {
        enableNightMode(true);  // ✅ تمرير true لتفعيل الوضع الليلي
        ui->nightmode->setText("☀️ Light Mode");
    } else {
        enableNightMode(false); // ✅ تمرير false لإلغاء الوضع الليلي
        ui->nightmode->setText("🌙 Night Mode");
    }
}

void MainWindow::enableNightMode(bool enable)
{
    if (enable) {
        this->setStyleSheet(
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
        this->setStyleSheet("");
    }
}


