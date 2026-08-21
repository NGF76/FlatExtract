#ifndef ISOTOOL_H
#define ISOTOOL_H
// ============================================================
// isotool.h
// دوال التعامل مع ملفات ISO (Xbox 360 / XDVDFS)
// ============================================================
#include <QString>
#include <QStringList>
#include <functional>

QStringList getIsoContents(const QString &isoPath, QString &errorMessage);

bool extractFileFromIso(const QString &isoPath, const QString &fileName, const QString &outputPath, QString &errorMessage);

// Callback signature: (current index, total count, current file name)
using ExtractProgressCallback = std::function<void(int, int, const QString&)>;

bool extractAllFromIso(const QString &isoPath, const QString &outputPath,
                       QString &errorMessage,
                       ExtractProgressCallback progressCallback = nullptr);

bool testLibCdio();

#endif // ISOTOOL_H
