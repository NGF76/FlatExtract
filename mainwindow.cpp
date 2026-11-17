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
#include "tarextract.h"
#include "externalstorage.h"
#include "chdtool.h"

#include <QApplication>
#include <QFile>
#include <QFileDialog>
#include <QWidget>
#include <QLineEdit>
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
#include <QMimeData>
#include <QDropEvent>
#include <QDragEnterEvent>
#include <QAction>
#include <QMenu>
#include <QStyle>
#include <QFileSystemModel>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QHeaderView>
#include <QFileIconProvider>
#include <QMimeDatabase>
#include <QHash>


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
    setupTreeViews();
    //setupUI();
    setupExternalStorageMenu();

    // ─── 3. ربط الأزرار الرئيسية ────────────────────────────
    connect(ui->addfiles, &QPushButton::clicked,
            this, &MainWindow::onChooseFileClicked);
    connect(ui->chooseadestination, &QPushButton::clicked,
            this, &MainWindow::onChooseDestinationClicked);
    connect(ui->destinationLineEdit, &QLineEdit::textEdited,
            this, [this](const QString &) {
                destinationUserSelected = true;
            });
    connect(ui->startextract, &QPushButton::clicked,
            this, &MainWindow::onExtractClicked);
    connect(ui->settings, &QPushButton::clicked,
            this, &MainWindow::onSettingsClicked);
    connect(ui->supportButton, &QPushButton::clicked,
            this, &MainWindow::onSupportClicked);
    // زر الحذف الملفات من القائمة
    //connect(ui->removeButton, &QPushButton::clicked, this, &MainWindow::onRemoveClicked);

    // ─── 4. زر Debug Console (تبديل الإظهار/الإخفاء) ────────
    connect(ui->debugconsole, &QPushButton::clicked, this, [this]() {
        debugDock->setVisible(!debugDock->isVisible());
    });
    // ---------------- السحب و الافلات ------------------
    ui->treeViewBefore->setAcceptDrops(true);
    setAcceptDrops(true);  // للنافذة الرئيسية


    // ─── 5. ربط إشارة Debug ─────────────────────────────────
    connect(this, &MainWindow::debugSignal,
            this, &MainWindow::appendDebugMessage);

    // ─── 6. الإعدادات الافتراضية للنافذة ────────────────────
    resize(1120, 700);
    setWindowTitle("FlatExtract");
    setWindowIcon(QIcon(":/Icon App/FlatExtract-Base.png"));
    ui->statusbar->setSizeGripEnabled(false);
    ui->startextract->setDefault(true);
    ui->startextract->setEnabled(false);
    ui->progressBar->setRange(0, 100);
    ui->progressBar->setValue(0);
    ui->progressBar->setFormat(QStringLiteral("%p%"));
    ui->progressBar->setVisible(true);

    connect(ui->filepathlineEdit, &QLineEdit::textChanged, this,
            [this](const QString &path) {
                ui->startextract->setEnabled(!path.trimmed().isEmpty());
            });

    //  تعيين مجلد افتراضي للوجهة (إذا لم يتم تعيينه سابقاً)
    QSettings startupSettings("NGF76", "FlatExtract");
    const bool rememberDestination =
            startupSettings.value("RememberDestination", true).toBool();
    QString initialDestination =
            rememberDestination ? startupSettings.value("LastDestination").toString()
                                : QString();
    if (initialDestination.trimmed().isEmpty())
        initialDestination = QDir::homePath() + "/Desktop/output";

    ui->destinationLineEdit->setText(initialDestination);
    updateOutputExplorer(initialDestination);
    showStatusMessage(ui->statusbar, tr("Ready"), 0);

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
    debugDock->setFeatures(QDockWidget::DockWidgetMovable);

    debugOutput = new QPlainTextEdit(this);
    debugOutput->setObjectName("debugOutput");
    debugOutput->setReadOnly(true);
    debugOutput->setMinimumHeight(140);
    debugOutput->setFont(QFont("Monospace", 10));
    debugOutput->setLineWrapMode(QPlainTextEdit::NoWrap);

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
// إعداد عارضات الشجرة (QTreeView) بأسلوب مستكشف الملفات
// ============================================================

void MainWindow::setupTreeViews()
{
    archiveContentsModel = new QStandardItemModel(this);
    archiveContentsModel->setHorizontalHeaderLabels({tr("Name"), tr("Size"), tr("Type")});

    ui->treeViewBefore->setModel(archiveContentsModel);
    ui->treeViewBefore->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->treeViewBefore->setRootIsDecorated(true);
    ui->treeViewBefore->setAllColumnsShowFocus(true);
    ui->treeViewBefore->setIndentation(18);
    ui->treeViewBefore->header()->setStretchLastSection(false);
    ui->treeViewBefore->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->treeViewBefore->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->treeViewBefore->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);

    outputFileSystemModel = new QFileSystemModel(this);
    outputFileSystemModel->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot);

    ui->treeViewAfter->setModel(outputFileSystemModel);
    ui->treeViewAfter->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->treeViewAfter->setRootIsDecorated(true);
    ui->treeViewAfter->setAllColumnsShowFocus(true);
    ui->treeViewAfter->setIndentation(18);
    ui->treeViewAfter->hideColumn(3); // Hide Date Modified; keep Name, Size, Type
    ui->treeViewAfter->header()->setStretchLastSection(false);
    ui->treeViewAfter->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->treeViewAfter->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->treeViewAfter->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
}

void MainWindow::populateArchiveTree(const QString &archivePath, const QStringList &entries)
{
    archiveContentsModel->removeRows(0, archiveContentsModel->rowCount());

    QFileIconProvider iconProvider;
    QMimeDatabase mimeDb;
    const QIcon folderIcon = iconProvider.icon(QFileIconProvider::Folder);
    const QIcon fileIcon = iconProvider.icon(QFileIconProvider::File);

    QFileInfo archiveInfo(archivePath);
    QStandardItem *rootNameItem = new QStandardItem(fileIcon, archiveInfo.fileName());
    QStandardItem *rootSizeItem = new QStandardItem(formatSize(archiveInfo.size()));
    QStandardItem *rootTypeItem = new QStandardItem(tr("Archive (%1 items)").arg(entries.size()));
    rootSizeItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    archiveContentsModel->appendRow({rootNameItem, rootSizeItem, rootTypeItem});

    if (entries.isEmpty()) {
        QStandardItem *emptyItem = new QStandardItem(tr("No files found."));
        rootNameItem->appendRow({emptyItem, new QStandardItem(QStringLiteral("—")), new QStandardItem(tr("Info"))});
        ui->treeViewBefore->expand(rootNameItem->index());
        return;
    }

    QHash<QString, QStandardItem *> folderNodes;

    for (const QString &rawEntry : entries) {
        QString cleaned = QDir::fromNativeSeparators(rawEntry).trimmed();
        while (cleaned.startsWith('/'))
            cleaned.remove(0, 1);
        if (cleaned.isEmpty())
            continue;

        const bool isExplicitDir = cleaned.endsWith('/');
        if (isExplicitDir)
            cleaned.chop(1);
        if (cleaned.isEmpty())
            continue;

        const QStringList parts = cleaned.split('/', Qt::SkipEmptyParts);
        QStandardItem *parentNode = rootNameItem;
        QString runningPath;

        for (int i = 0; i < parts.size(); ++i) {
            const QString &part = parts.at(i);
            runningPath = runningPath.isEmpty() ? part : (runningPath + '/' + part);
            const bool isDir = (i < parts.size() - 1) || isExplicitDir;

            if (isDir) {
                if (!folderNodes.contains(runningPath)) {
                    QStandardItem *dirName = new QStandardItem(folderIcon, part);
                    QStandardItem *dirSize = new QStandardItem(QStringLiteral("—"));
                    QStandardItem *dirType = new QStandardItem(tr("Folder"));
                    dirSize->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
                    parentNode->appendRow({dirName, dirSize, dirType});
                    folderNodes.insert(runningPath, dirName);
                }
                parentNode = folderNodes.value(runningPath);
            } else {
                const QMimeType mime = mimeDb.mimeTypeForFile(part, QMimeDatabase::MatchExtension);
                QString typeDesc = mime.isValid() && !mime.comment().isEmpty()
                        ? mime.comment()
                        : tr("File");

                QStandardItem *fileName = new QStandardItem(fileIcon, part);
                QStandardItem *fileSize = new QStandardItem(QStringLiteral("—"));
                QStandardItem *fileType = new QStandardItem(typeDesc);
                fileSize->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
                parentNode->appendRow({fileName, fileSize, fileType});
            }
        }
    }

    ui->treeViewBefore->expand(rootNameItem->index());
}

void MainWindow::updateOutputExplorer(const QString &outputPath)
{
    if (!outputFileSystemModel || outputPath.trimmed().isEmpty())
        return;

    QDir dir(outputPath);
    if (dir.exists()) {
        const QModelIndex rootIdx = outputFileSystemModel->setRootPath(dir.absolutePath());
        ui->treeViewAfter->setRootIndex(rootIdx);
    } else {
        ui->treeViewAfter->setRootIndex(QModelIndex());
    }
}


// دالة تنسيق الحجم
QString MainWindow::formatSize(qint64 size){
    if (size < 1024){
        return QString::number(size) + " B";
    }
    else if(size < 1024 * 1024){
        return QString::number(size / 1024.0, 'f', 1) + "KB";
    }
    else if (size < 1024 * 1024 * 1024) {
        return QString::number(size / (1024.0 * 1024.0), 'f', 1) + "MB";
    }
    else{
        return QString::number(size / (1024.0 * 1024.0 * 1024.0), 'f', 1) + " GB";
    }
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
        tr("Select a compressed file, ISO, or GameCube image."),
        QDir::homePath(),
        tr("All Supported (*.zip *.tar *.tgz *.tar.gz *.tbz *.tar.bz *.tbz2 *.tar.bz2 *.tar.xz *.iso *.gcm);;"
           "ZIP Files (*.zip);;"
           "TAR Files (*.tar *.tgz *.tar.gz *.tbz *.tar.bz *.tbz2 *.tar.bz2 *.tar.xz);;"
           "Disc Images (*.iso *.gcm);;All Files (*)")
        );

    if (filePath.isEmpty()) {
        debugError(tr("No file selected."));
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
    if (!destinationUserSelected || ui->destinationLineEdit->text().trimmed().isEmpty())
        ui->destinationLineEdit->setText(QDir::homePath() + "/Desktop/" + baseName);
    archiveContentsModel->removeRows(0, archiveContentsModel->rowCount());
    updateOutputExplorer(ui->destinationLineEdit->text());

    // ─── 4. قراءة المحتويات حسب نوع الملف ────────────────
    QString suffix = QFileInfo(filePath).suffix().toLower();
    QStringList files;
    QString errorMessage;


    if (suffix == "iso" || suffix == "gcm")
    {
        files = getIsoContents(filePath, errorMessage);
    } else if (suffix == "zip")
    {
        files = getArchiveContents(filePath, errorMessage);
    } else if (isSupportedTarArchive(filePath))
    {
        files = getTarArchiveContents(filePath, errorMessage);
    } else
    {
        errorMessage = tr("Unsupported archive format.");
    }

    if (!errorMessage.isEmpty()) {
        ui->startextract->setEnabled(false);
        QMessageBox::warning(this, tr("Error"), errorMessage);
        debugFunctionEnd("onChooseFileClicked");
        return;
    }


    // ─── 5. عرض النتائج في شجرة الملفات ─────────────────
    populateArchiveTree(filePath, files);

    // ─── 6. تحديث شريط الحالة ────────────────────────────
    showStatusMessage(ui->statusbar,
                      tr("%1 file(s) loaded").arg(files.size()), 5000);

    debugFileList("Files in archive", files);
    debugFunctionEnd("onChooseFileClicked");
}

// ============================================================
// اختيار مجلد الوجهة (Destination)
// ============================================================

void MainWindow::onChooseDestinationClicked()
{
    debugFunctionStart("onChooseDestinationClicked");

    // 1. فتح نافذة اختيار مجلد، بدءاً من الوجهة الحالية إن وُجدت
    const QString startDirectory = nearestExistingDirectory(ui->destinationLineEdit->text());
    QFileDialog dialog(this, tr("Choose a Destination"), startDirectory);
    dialog.setOption(QFileDialog::ShowDirsOnly, true);
    dialog.setOption(QFileDialog::DontUseNativeDialog, true);
    dialog.setFileMode(QFileDialog::Directory);
    dialog.setAcceptMode(QFileDialog::AcceptOpen);

    // إضافة وسائط التخزين الخارجية المثبتة إلى الشريط الجانبي
    QList<QUrl> places = dialog.sidebarUrls();
    const QList<ExternalStorageVolume> volumes = externalStorageVolumes();
    for (const ExternalStorageVolume &volume : volumes) {
        const QUrl url = QUrl::fromLocalFile(volume.rootPath);
        if (!places.contains(url))
            places.append(url);
    }
    dialog.setSidebarUrls(places);

    // 2. إذا اختار المستخدم مجلداً
    QString folderPath;
    if (dialog.exec() == QDialog::Accepted) {
        const QStringList selected = dialog.selectedFiles();
        if (!selected.isEmpty())
            folderPath = selected.constFirst();
    }

    if (!folderPath.isEmpty()) {
        // عرض المسار في حقل النص
        destinationUserSelected = true;
        ui->destinationLineEdit->setText(QDir::toNativeSeparators(folderPath));

        // عرض المجلد في مستكشف المخرجات
        updateOutputExplorer(folderPath);

        // تسجيل في الـ Debug
        debugVariable("Destination Folder", folderPath);
    }

    debugFunctionEnd("onChooseDestinationClicked");
}

// ============================================================
// التخزين الخارجي (USB / SD card / أقراص خارجية)
// ============================================================

void MainWindow::setupExternalStorageMenu()
{
    externalStorageMenu = new QMenu(this);
    externalStorageMenu->setToolTipsVisible(true);
    ui->externalStorageButton->setMenu(externalStorageMenu);

    QIcon driveIcon(QStringLiteral(":/ui/icons/drive.png"));
    if (driveIcon.isNull())
        driveIcon = style()->standardIcon(QStyle::SP_DriveHDIcon);
    ui->externalStorageButton->setIcon(driveIcon);

    connect(externalStorageMenu, &QMenu::aboutToShow,
            this, &MainWindow::refreshExternalStorageMenu);
}

void MainWindow::refreshExternalStorageMenu()
{
    externalStorageMenu->clear();

    QAction *headingAction = externalStorageMenu->addAction(tr("External storage"));
    headingAction->setEnabled(false);
    headingAction->setStatusTip(tr("Mounted USB drives, SD cards, and external disks"));

    const QList<ExternalStorageVolume> volumes = externalStorageVolumes();
    if (volumes.isEmpty()) {
        QAction *emptyAction = externalStorageMenu->addAction(tr("No external storage detected"));
        emptyAction->setEnabled(false);
    } else {
        for (const ExternalStorageVolume &volume : volumes) {
            QString detail;
            if (volume.readOnly) {
                detail = volume.bytesTotal > 0
                        ? tr("read-only · %1").arg(formatSize(volume.bytesTotal))
                        : tr("read-only");
            } else if (volume.bytesAvailable >= 0) {
                detail = tr("%1 free of %2")
                        .arg(formatSize(volume.bytesAvailable), formatSize(volume.bytesTotal));
            } else {
                detail = tr("mounted");
            }

            QAction *action = externalStorageMenu->addAction(
                QIcon(QStringLiteral(":/ui/icons/drive.png")),
                tr("%1 — %2").arg(volume.displayName, detail));
            action->setToolTip(tr("Mount point: %1\nDevice: %2\nFile system: %3")
                               .arg(volume.rootPath, volume.device, volume.fileSystem));
            action->setStatusTip(volume.rootPath);

            if (volume.readOnly) {
                action->setEnabled(false);
            } else {
                connect(action, &QAction::triggered, this, [this, path = volume.rootPath]() {
                    applyExternalStorageDestination(path);
                });
            }
        }
    }

    externalStorageMenu->addSeparator();
    QAction *browseAction = externalStorageMenu->addAction(tr("Browse for a folder…"));
    connect(browseAction, &QAction::triggered,
            this, &MainWindow::onChooseDestinationClicked);
}

void MainWindow::applyExternalStorageDestination(const QString &path)
{
    if (path.isEmpty())
        return;

    const QString nativePath = QDir::toNativeSeparators(path);
    destinationUserSelected = true;
    ui->destinationLineEdit->setText(nativePath);

    updateOutputExplorer(nativePath);

    showStatusMessage(ui->statusbar, tr("Destination set to %1").arg(nativePath), 5000);
    debugVariable("Destination Folder", nativePath);
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
        debugError("No file selected.");
        QMessageBox::warning(this, tr("Error"), tr("Please select a compressed file first."));
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
    ui->startextract->setText(tr("Extracting..."));
    ui->addfiles->setEnabled(false);
    ui->chooseadestination->setEnabled(false);
    ui->externalStorageButton->setEnabled(false);
    ui->filepathlineEdit->setEnabled(false);
    ui->destinationLineEdit->setEnabled(false);

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
                ui->addfiles->setEnabled(true);
                ui->chooseadestination->setEnabled(true);
                ui->externalStorageButton->setEnabled(true);
                ui->filepathlineEdit->setEnabled(true);
                ui->destinationLineEdit->setEnabled(true);
                ui->startextract->setEnabled(!ui->filepathlineEdit->text().trimmed().isEmpty());
                ui->startextract->setText(tr("Start Extraction"));

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
                    // -----------------  عرض الملفات و المجلدات المستخرجة ----------------------
                    updateOutputExplorer(outputPath);

                    QMessageBox::information(this, tr("The operation was completed successfully."),
                                             tr("The files were successfully extracted to:\n%1").arg(outputPath));

                } else {
                    ui->statusbar->showMessage(tr("Extraction failed: %1").arg(message), 5000);
                    QMessageBox::critical(this, tr("Error"), message);
                }

                // تنظيف الخيط
                sender()->deleteLater();
                debugFunctionEnd("onExtractClicked");
            });

    // ─── 7. ربط تحديث التقدم ───────────────────────────────
    connect(thread, &ExtractThread::progressUpdated, this,
            [this](int current, int total) {
                updateProgressBar(ui->progressBar, current, total);
                if (total > 0) {
                    ui->statusbar->showMessage(tr("Extracting %1/%2").arg(current).arg(total), 0);
                } else {
                    ui->statusbar->showMessage(tr("Extracting %1 file(s)...").arg(current), 0);
                }
            });

    // ─── 8. تشغيل الخيط ─────────────────────────────────────
    thread->start();
    debugFunctionEnd("onExtractClicked");
}

// ============================================================
// تصميم الواجهة (UI Styling)
// ============================================================



// ============================================================
// الإعدادات (Settings)
// ============================================================

void MainWindow::onSettingsClicked()
{
    SettingsDialog dialog(this);
    dialog.setWindowIcon(windowIcon());

    connect(&dialog, &SettingsDialog::settingsApplied, this, &MainWindow::applySettings);

    dialog.exec();
}

// ============================================================
// تطبيق الإعدادات المحفوظة
// ============================================================

void MainWindow::applySettings()
{
    QSettings settings("NGF76", "FlatExtract");

    bool nightMode = settings.value("NightMode", true).toBool();
    enableNightMode(nightMode);

    // ─── حجم الخط / تكامل الواجهة ───
    const int fontSizeIndex = settings.value("FontSizeIndex", 1).toInt();
    int pointSize = 10;
    switch (fontSizeIndex) {
        case 0: pointSize = 10; break; // Small
        case 2: pointSize = 14; break; // Large
        default: pointSize = 12; break;// Medium
    }
    QFont appFont = QApplication::font();
    appFont.setPointSize(pointSize);
    QApplication::setFont(appFont);

    // ─── إظهار وحدة التصحيح عند البدء ───
    const bool debugConsole = settings.value("DebugConsoleOnStartup", false).toBool();
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
    const QString themePath = enable ? ":/style.qss" : ":/light.qss";
    QFile themeFile(themePath);

    if (!themeFile.open(QFile::ReadOnly | QFile::Text)) {
        qWarning() << "Failed to load theme:" << themePath;
        return;
    }

    qApp->setStyleSheet(QString::fromUtf8(themeFile.readAll()));
    isNightMode = enable;
}

// ------------------- رابط الدعم حق الموقع ----------------------------
void MainWindow::onSupportClicked()
{
    // ✅ رابط حساباتي (غيِّر الرابط إلى رابطك)
    QString url = "https://ngf76.github.io/";  // أو رابط حسابك على تويتر، يوتيوب، إلخ

    // فتح الرابط في المتصفح الافتراضي
    if (!QDesktopServices::openUrl(QUrl(url))) {
        QMessageBox::warning(this, "Error", "Unable to open the link.");
    }
}

// ============================================================
// دعم السحب والإفلات (Drag & Drop)
// ============================================================

//void MainWindow::dragEnterEvent(QDragEnterEvent *event)
//{
    // قبول السحب فقط إذا كان يحتوي على ملفات
    //if (event->mimeData()->hasUrls()) {
      //  event->acceptProposedAction();
    //}
//}

void MainWindow::dropEvent(QDropEvent *event)
{
    // الحصول على الملفات المسحوبة
    QList<QUrl> urls = event->mimeData()->urls();
    for (const QUrl &url : urls) {
        QString filePath = url.toLocalFile();
        if (!filePath.isEmpty()) {
            ui->filepathlineEdit->setText(filePath);
            populateArchiveTree(filePath, QStringList());
            break;
        }
    }
    event->acceptProposedAction();
}

// حذف الملفات من القائمة قبل الاستخراج
void MainWindow::onRemoveClicked()
{
    QModelIndex current = ui->treeViewBefore->currentIndex();
    if (current.isValid()) {
        archiveContentsModel->removeRow(current.row(), current.parent());
    }
}




