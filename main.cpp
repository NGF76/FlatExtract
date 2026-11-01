// ============================================================
// main.cpp
// نقطة الدخول الرئيسية للتطبيق
// ============================================================

#include <QFont>
#include <QFontDatabase>
#include <QApplication>
#include <QFile>
#include <QIcon>
#include <QDebug>
#include <QLabel>
#include <QSettings>
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
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

    // 1. إنشاء كائن التطبيق
    QApplication app(argc, argv);
    QApplication::setStyle("Fusion");

    // 2. تحميل خط Inter المرفق مع التطبيق
    QString fontFamily;
    const QStringList fontFiles = {
        ":/fonts/Inter/Inter-VariableFont_opsz,wght.ttf",
        ":/fonts/Inter/Inter-Italic-VariableFont_opsz,wght.ttf"
    };

    for (const QString &fontFile : fontFiles) {
        const int fontId = QFontDatabase::addApplicationFont(fontFile);
        if (fontId == -1) {
            qWarning() << "Failed to load font:" << fontFile;
            continue;
        }

        if (fontFamily.isEmpty()) {
            const QStringList families = QFontDatabase::applicationFontFamilies(fontId);
            if (!families.isEmpty()) {
                fontFamily = families.first();
            }
        }
    }

    if (!fontFamily.isEmpty()) {
        QFont appFont(fontFamily);
        appFont.setPointSize(10);
        QApplication::setFont(appFont);
    }

    // 3. تحميل السمة المحفوظة (الوضع الداكن افتراضياً)
    QSettings settings("NGF76", "FlatExtract");
    const bool darkTheme = settings.value("NightMode", true).toBool();
    QFile styleFile(darkTheme ? ":/style.qss" : ":/light.qss");

    if(styleFile.open(QFile::ReadOnly | QFile::Text)){
        QString style = QString::fromUtf8(styleFile.readAll());
        qApp->setStyleSheet(style);
        styleFile.close();
    }else {
        qDebug() << "Failed to load theme stylesheet";
    }

    // 4. تشغيل حلقة الأحداث
    MainWindow w;
    w.show();
    return app.exec();
}


