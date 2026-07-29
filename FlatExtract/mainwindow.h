#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
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

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
//==================================
//private function for ui
//==================================
private:
        Ui::MainWindow *ui;
    QString find7zExecutable();
//===============================================================================================
// private slots mean Driver The Function for mainwindow.h for mainwindow.cpp file to use function
//================================================================================================
private slots:
        void onChooseFileClicked();
        void onExtractClicked();
};


#endif // MAINWINDOW_H
