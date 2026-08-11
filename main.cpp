// ============================================================
// main.cpp
// نقطة الدخول الرئيسية للتطبيق
// ============================================================

#include <QApplication>
#include "mainwindow.h"
#include "Xtool.h"
#include "xex_parser.h"
#include "iso_reader.h"

/**
 * @brief الدالة الرئيسية (Entry Point)
 * @param argc عدد الوسائط
 * @param argv قائمة الوسائط
 * @return 0 عند النجاح، قيمة غير صفرية عند الخطأ
 */
int main(int argc, char *argv[])
{
    // 1. إنشاء كائن التطبيق
    QApplication app(argc, argv);

    // 2. إنشاء النافذة الرئيسية عبر الدالة الثابتة
    MainWindow *mainWindow = MainWindow::createInstance();
    mainWindow->show();

    // 3. تشغيل حلقة الأحداث
    return app.exec();
}
