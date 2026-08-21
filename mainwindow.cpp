// ============================================================
// mainwindow.cpp
// النافذة الرئيسية للتطبيق
// ============================================================

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "tool.h"
#include "settingsdialog.h"
#include "debughelper.h"
#include "progresshelper.h"
#include "isotool.h"
#include "extractthread.h"

#include <QFileDialog>
#include <QSettings>
#include <QMessageBox>
#include <QDir>
#include <QFileInfo>
#include <QDirIterator>
#include <QStatusBar>
#include <QDebug>
#include <QIcon>
#include <QThread>
#include <QDesktopServices>
#include <QUrl>


// ============================================================
// معالج رسائل Debug (عام)
// ============================================================

static void debugMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QString prefix;
    switch (type) {
    case QtDebugMsg:    prefix = "[DEBUG]"; break;
    case QtWarningMsg:  prefix = "[WARNING]"; break;
    case QtCriticalMsg: prefix = "[CRITICAL]"; break;
    case QtFatalMsg:    prefix = "[FATAL]"; break;
    default:            prefix = "[INFO]"; break;
    }

    fprintf(stderr, "%s %s\n", prefix.toUtf8().constData(), msg.toUtf8().constData());

    MainWindow *mainWindow = qobject_cast<MainWindow*>(qApp->activeWindow());
    if (mainWindow) {
        emit mainWindow->debugSignal(QString("%1 %2").arg(prefix).arg(msg));
    }

    if (type == QtFatalMsg) abort();
}

// ============================================================
// المُنشئ (Constructor)
// ============================================================

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // ─── 1. إعداد معالج Debug ──────────────────────────────
    qInstallMessageHandler(debugMessageHandler);

    // ─── 2. إعداد الواجهات ──────────────────────────────────
    setupDebugConsole();
    setupUI();

    // ─── 3. ربط الأزرار الرئيسية ────────────────────────────
    connect(ui->fileselection, &QPushButton::clicked,
            this, &MainWindow::onChooseFileClicked);
    connect(ui->chooseadestination, &QPushButton::clicked,
            this, &MainWindow::onChooseDestinationClicked);
    connect(ui->startextract, &QPushButton::clicked,
            this, &MainWindow::onExtractClicked);
    connect(ui->settings, &QPushButton::clicked,
            this, &MainWindow::onSettingsClicked);
    connect(ui->supportButton, &QPushButton::clicked,
            this, &MainWindow::onSupportClicked);

    // ─── 4. زر Debug Console (تبديل الإظهار/الإخفاء) ────────
    connect(ui->debugconsole, &QPushButton::clicked, this, [this]() {
        debugDock->setVisible(!debugDock->isVisible());
    });

    // ─── 5. ربط إشارة Debug ─────────────────────────────────
    connect(this, &MainWindow::debugSignal,
            this, &MainWindow::appendDebugMessage);

    // ─── 6. الإعدادات الافتراضية للنافذة ────────────────────
    setFixedSize(900, 600);
    setWindowTitle("FlatExtract");
    setWindowIcon(QIcon(":/Icon App/FlatExtract-Base.png"));

    // ✅ تعيين مجلد افتراضي للوجهة (إذا لم يتم تعيينه سابقاً)
    ui->destinationLineEdit->setText(QDir::homePath() + "/Desktop/output");

    // ─── 7. تطبيق الإعدادات المحفوظة ────────────────────────
    applySettings();
}

// ============================================================
// المُدمر
// ============================================================

MainWindow::~MainWindow()
{
    delete ui;
}

// ============================================================
// دوال عامة
// ============================================================

QStatusBar* MainWindow::getStatusBar() const { return ui->statusbar; }
QProgressBar* MainWindow::getProgressBar() const { return ui->progressBar; }

// ============================================================
// دوال Debug Console
// ============================================================

void MainWindow::setupDebugConsole()
{
    debugDock = new QDockWidget("Debug Console", this);
    debugDock->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);

    debugOutput = new QPlainTextEdit(this);
    debugOutput->setReadOnly(true);
    debugOutput->setFont(QFont("Monospace", 10));
    debugOutput->setStyleSheet("background-color: #1e1e1e; color: #d4d4d4;");

    debugDock->setWidget(debugOutput);
    addDockWidget(Qt::BottomDockWidgetArea, debugDock);
    debugDock->setVisible(false);

    connect(this, &MainWindow::debugSignal, this, &MainWindow::appendDebugMessage);
}

void MainWindow::appendDebugMessage(const QString &msg)
{
    if (debugOutput) debugOutput->appendPlainText(msg);
}

// ============================================================
// فتحات الأزرار
// ============================================================

// ============================================================
// اختيار الملف وعرض محتوياته
// ============================================================

void MainWindow::onChooseFileClicked()
{
    debugFunctionStart("onChooseFileClicked");

    // ─── 1. اختيار الملف ──────────────────────────────────
    QString filePath = QFileDialog::getOpenFileName(
        this,
        tr("اختر ملف مضغوط أو ISO"),
        QDir::homePath(),
        tr("All Supported (*.zip *.7z *.tar.gz *.tar.bz2 *.tar.xz *.iso);;"
           "ZIP Files (*.zip);;ISO Files (*.iso);;All Files (*)")
        );

    if (filePath.isEmpty()) {
        debugError(tr("لم يتم اختيار ملف"));
        debugFunctionEnd("onChooseFileClicked");
        return;
    }

    // ─── 2. عرض المسار في الحقل ──────────────────────────
    ui->filepathlineEdit->setText(filePath);
    debugVariable("File Path", filePath);
    debugFileInfo(filePath);

    // ─── 3. تحديث مجلد الوجهة الافتراضي ──────────────────
    QFileInfo fileInfo(filePath);
    QString baseName = fileInfo.baseName();
    ui->destinationLineEdit->setText(QDir::homePath() + "/Desktop/" + baseName);

    // ─── 4. قراءة المحتويات حسب نوع الملف ────────────────
    QString suffix = QFileInfo(filePath).suffix().toLower();
    QStringList files;
    QString errorMessage;

    if (suffix == "iso") {
        files = getIsoContents(filePath, errorMessage);
    } else {
        files = getArchiveContents(filePath, errorMessage);
    }

    if (!errorMessage.isEmpty()) {
        QMessageBox::warning(this, tr("خطأ"), errorMessage);
        debugFunctionEnd("onChooseFileClicked");
        return;
    }

    // ─── 5. عرض النتائج في القائمة ───────────────────────
    ui->listWidgetBefore->clear();
    ui->listWidgetBefore->addItem(tr("📁 %1").arg(QFileInfo(filePath).fileName()));

    if (files.isEmpty()) {
        ui->listWidgetBefore->addItem(tr("⚠️ لا توجد ملفات"));
    } else {
        ui->listWidgetBefore->addItem(tr("📊 عدد الملفات: %1").arg(files.size()));
        for (const QString &file : files) {
            ui->listWidgetBefore->addItem(tr("📄 %1").arg(file));
        }
    }

    // ─── 6. تحديث شريط الحالة ────────────────────────────
    showStatusMessage(ui->statusbar,
                      tr("تم تحميل %1 ملف").arg(files.size()), 5000);

    debugFileList("Files in archive", files);
    debugFunctionEnd("onChooseFileClicked");
}

// ============================================================
// اختيار مجلد الوجهة (Destination)
// ============================================================

void MainWindow::onChooseDestinationClicked()
{
    debugFunctionStart("onChooseDestinationClicked");

    // 1. فتح نافذة اختيار مجلد
    QString folderPath = QFileDialog::getExistingDirectory(
        this,
        tr("Choose a Destination"),      // عنوان النافذة (قابل للترجمة)
        QDir::homePath(),                // المجلد الافتراضي (مجلد المستخدم)
        QFileDialog::ShowDirsOnly        // عرض المجلدات فقط
        );

    // 2. إذا اختار المستخدم مجلداً
    if (!folderPath.isEmpty()) {
        // عرض المسار في حقل النص
        ui->destinationLineEdit->setText(folderPath);

        // عرض المجلد في القائمة (جاهز للاستخراج)
        ui->listWidgetAfter->clear();
        ui->listWidgetAfter->addItem(tr("📁 %1 جاهز للاستخراج").arg(folderPath));

        // تسجيل في الـ Debug
        debugVariable("Destination Folder", folderPath);
    }

    debugFunctionEnd("onChooseDestinationClicked");
}


// ============================================================
// استخراج الملفات (مع خيط منفصل)
// ============================================================

void MainWindow::onExtractClicked()
{
    debugFunctionStart("onExtractClicked");

    // ─── 1. قراءة مسار الملف ──────────────────────────────
    QString filePath = ui->filepathlineEdit->text();
    if (filePath.isEmpty()) {
        debugError("لم يتم اختيار ملف");
        QMessageBox::warning(this, tr("خطأ"), tr("الرجاء اختيار ملف مضغوط أولاً"));
        debugFunctionEnd("onExtractClicked");
        return;
    }

    // ─── 2. تحديد مجلد الوجهة ──────────────────────────────
    QString outputPath = ui->destinationLineEdit->text();
    if (outputPath.isEmpty()) {
        // ✅ استخدم اسم الملف كمجلد افتراضي
        QFileInfo fileInfo(filePath);
        QString baseName = fileInfo.baseName();
        outputPath = QDir::homePath() + "/Desktop/" + baseName;
    }
    debugVariable("Output Path", outputPath);

    // ─── 3. تعطيل الزر ومنع التكرار ─────────────────────────
    ui->startextract->setEnabled(false);
    ui->startextract->setText(tr(" Extracting... "));

    // ─── 4. تهيئة شريط التقدم ──────────────────────────────
    initProgressBar(ui->progressBar, 0);
    setProgressBarVisible(ui->progressBar, true);
    showExtractStart(ui->statusbar);

    // ─── 5. إنشاء وتجهيز الخيط ─────────────────────────────
    ExtractThread *thread = new ExtractThread(this);
    thread->setData(filePath, outputPath);

    // ─── 6. ربط إشارة الانتهاء ─────────────────────────────
    connect(thread, &ExtractThread::finished, this,
            [this, outputPath](bool success, const QString &message) {

                // إعادة تفعيل الزر
                ui->startextract->setEnabled(true);
                ui->startextract->setText(tr(" Start Extract "));

                // إخفاء شريط التقدم
                resetProgressBar(ui->progressBar);

                if (success) {
                    // عرض الملفات المستخرجة
                    QDir extractedDir(outputPath);
                    QStringList files = extractedDir.entryList(QDir::Files | QDir::NoDotAndDotDot);

                    if (files.isEmpty()) {
                        QDirIterator it(outputPath, QDir::Files | QDir::NoDotAndDotDot,
                                        QDirIterator::Subdirectories);
                        while (it.hasNext()) {
                            it.next();
                            files << it.fileInfo().absoluteFilePath();
                        }
                    }

                    showExtractEnd(ui->statusbar, files.size());

                    ui->listWidgetAfter->clear();
                    ui->listWidgetAfter->addItem(tr("📁 %1 (تمت العملية)").arg(outputPath));

                    if (files.isEmpty()) {
                        ui->listWidgetAfter->addItem(tr("⚠️ لا توجد ملفات مستخرجة"));
                    } else {
                        for (const QString &file : files) {
                            ui->listWidgetAfter->addItem(tr("📄 %1 (تمت العملية)").arg(file));
                        }
                    }

                    QMessageBox::information(this, tr("تمت العملية بنجاح "),
                                             tr("تم استخراج الملفات بنجاح إلى:\n%1").arg(outputPath));

                } else {
                    ui->statusbar->showMessage(tr("❌ فشل الاستخراج: %1").arg(message), 5000);
                    QMessageBox::critical(this, tr("خطأ"), message);
                }

                // تنظيف الخيط
                sender()->deleteLater();
                debugFunctionEnd("onExtractClicked");
            });

    // ─── 7. ربط تحديث التقدم ───────────────────────────────
    connect(thread, &ExtractThread::progressUpdated, this,
            [this](int current, int total) {
                updateProgressBar(ui->progressBar, current, total);
                ui->statusbar->showMessage(tr("استخراج %1/%2").arg(current).arg(total), 0);
            });

    // ─── 8. تشغيل الخيط ─────────────────────────────────────
    thread->start();
    debugFunctionEnd("onExtractClicked");
}

// ============================================================
// البحث عن 7z
// ============================================================

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

// ============================================================
// تصميم الواجهة (UI Styling)
// ============================================================

void MainWindow::setupUI()
{
    // زر الاستخراج (أخضر)
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

    ui->fileselection->setStyleSheet(
        "QPushButton { background-color: #3498db; color: white; }"
        "QPushButton:hover { background-color: #2980b9; }"
        );

    ui->chooseadestination->setStyleSheet(
        "QPushButton { background-color: #3498db; color: white; }"
        "QPushButton:hover { background-color: #2980b9; }"
        );

    ui->debugconsole->setStyleSheet(
        "QPushButton { background-color: #FF0000; color: white; padding: 6px; }"
        "QPushButton:hover { background-color: #cc0000; }"
        );
}

// ============================================================
// الإعدادات (Settings)
// ============================================================

void MainWindow::onSettingsClicked()
{
    SettingsDialog dialog(this);
    dialog.resize(this->size());

    connect(&dialog, &SettingsDialog::nightModeToggled, this, &MainWindow::enableNightMode);
    connect(&dialog, &SettingsDialog::settingsApplied, this, &MainWindow::applySettings);

    dialog.exec();
}

// ============================================================
// تطبيق الإعدادات المحفوظة
// ============================================================

void MainWindow::applySettings()
{
    qDebug() << "🔵 [3] applySettings called!";

    QSettings settings("NGF76", "FlatExtract");

    bool nightMode = settings.value("NightMode", false).toBool();
    enableNightMode(nightMode);

    bool debugConsole = settings.value("DebugConsole", false).toBool();
    if (debugDock) {
        debugDock->setVisible(debugConsole);
    }
}

// ============================================================
// دوال Night Mode
// ============================================================

void MainWindow::toggleNightMode()
{
    isNightMode = !isNightMode;
    enableNightMode(isNightMode);
}

void MainWindow::enableNightMode(bool enable)
{
    qDebug() << "🔵 [2] enableNightMode called with:" << enable;

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

void MainWindow::onSupportClicked()
{
    // ✅ رابط حسابك (غيِّر الرابط إلى رابطك)
    QString url = "https://ngf76.github.io/";  // أو رابط حسابك على تويتر، يوتيوب، إلخ

    // فتح الرابط في المتصفح الافتراضي
    if (!QDesktopServices::openUrl(QUrl(url))) {
        QMessageBox::warning(this, "خطأ", "تعذر فتح الرابط");
    }
}




