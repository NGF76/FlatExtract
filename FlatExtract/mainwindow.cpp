#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "tool.h"
#include "multiarchive.h"
#include <QPushButton>
#include <QFileDialog>
#include <QProcess>
#include <QListWidget>
#include <QProgressBar>
#include <QMessageBox>
#include <QString>
#include <QDir>
#include <QMainWindow>
#include <QStringList>
#include <QLineEdit>
#include <QFileInfo>
#include <QDebug>
#include <QArgument>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(ui->choseiso, &QPushButton::clicked, this, &MainWindow::onChooseFileClicked);
    //==========================
    // create
    //==========================


    //================================
    // connect list view
    //================================


}

MainWindow::~MainWindow()
{
    delete ui;
}
//===========================================
// function of Extract
//===========================================
void MainWindow::onExtractClicked()
{
    QString filePath = ui->lineEdit->text();

    if (filePath.isEmpty()){
        QMessageBox::warning(this, "Error" , "The operation failed" , "Pease Select Your Fille");
        return;
    }
    //==============================
    // Extract Fille
    //===============================
    QDir outputDir = QFileInfo(filePath).absoluteDir();
    QString outputPath = outputDir.absolutePath();
    QDir().mkpath(outputPath); //make file if u not have a file

    QProcess process;
    process.start("7z", {"x", filePath, "-o" + outputPath});
    process.waitForFinished();
    //======================================
    // show file after Extract
    //======================================
    QDir extractedDir(outputPath);
    QStringList files = extractedDir.entryList(QDir::Files | QDir::NoDotAndDotDot);

    //==================
    // use tool.h
    //==================
    QString errorMessage;
    bool success = extractArchive(filePath, outputPath, errorMessage);

    if (!success) {
        QMessageBox::critical(this, "خطأ", errorMessage);
        return;
    }
}
//==================================================
// choise file function
//==================================================
 void MainWindow::onChooseFileClicked()
    {
     QString errorMessage;
    QString filePath = QFileDialog::getOpenFileName(this, "All Files (*)");
    if (filePath.isEmpty()) return;
    ui->choseiso->setText("");
    //============================================
    // Show file 7z or iso files
    //============================================
    QProcess process;
    QStringList arguments;
    arguments << "1"<< filePath;
    process.start("7z", arguments);
    if(!process.waitForFinished()){
        return;
    }
    process.waitForFinished();
    arguments <<"1" << filePath;
    QString output = process.readAllStandardOutput();
    //==========================================
    //Show Name File to Extract
    //=========================================
    QStringList files;
    //================
    //qDebug
    //===============
    qDebug()<< "Number of Files" << files.size();
    qDebug()<< "Files"<< files;
    qDebug()<< "===Full 7z Output===";
    qDebug()<< output;
    qDebug()<< "===============";
  //=====================================
  //Widget
  //=====================================

    ui->listWidgetBefore->clear();

    ui->listWidgetBefore->addItem("━━━━━━━━━━━━━━━━━━━━");
    ui->listWidgetBefore->addItem("📦 " + QFileInfo(filePath).fileName());
    ui->listWidgetBefore->addItem("━━━━━━━━━━━━━━━━━━━━");

    files = getArchiveContents(filePath, errorMessage);

    if (files.isEmpty()) {
        ui->listWidgetBefore->addItem("⚠️ لا توجد ملفات");
    } else {
        ui->listWidgetBefore->addItem("📊 عدد الملفات: " + QString::number(files.size()));
        ui->listWidgetBefore->addItem("━━━━━━━━━━━━━━━━━━━━");

        for (const QString &file : files) {
            ui->listWidgetBefore->addItem("📄 " + file);
        }
        for (const QString &filePath : filePath){
            QString fileName = QFileInfo(filePath).fileName();
            ui->listWidgetBefore->addItem("📦 " + fileName);
        }
    }
    QStringList lines = output.split("\n");
    for (const QString &line : lines){
        if (line.contains(".") && !line.contains("Date") && !line.contains("_________")){
        }
     }
     //==============
     //use tool.h
     //==============
     QStringList filesContents = getArchiveContents(filePath, errorMessage);

     if (!errorMessage.isEmpty()) {
         QMessageBox::warning(this, "خطأ", errorMessage);
         return;
     }
     //==================
     //Statusbar
     //==================s
     ui->statusbar->showMessage(" Selected " + QFileInfo(filePath).fileName(),5000);
     ui->statusbar->showMessage("Number of Selected Files" + QString::number(filePath.size()) + "Files", 7000);
}

    //================================================================
    // Function of Find 7z or gz File on Extract
    //=================================================================
    QString MainWindow::find7zExecutable() {
        QStringList possibleNames = {"7z","7za","7zr","gz"};
        QStringList searchPaths = {"/usr/bin/", "/usr/local/bin/"};

        for (const QString &name : possibleNames){
            for(const QString &path : searchPaths){
                QFileInfo file(path + name);
                if(file.exists() && file.isExecutable()){
                    return file.absoluteFilePath();
                }
            }
        }
        return "";
    }
//=======================
