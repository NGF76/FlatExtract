#ifndef ISOTOOL_H
#define ISOTOOL_H
// ============================================================
// isotool.h
// دوال التعامل مع ملفات ISO (Xbox 360 / XDVDFS)
// ============================================================
#include <QString>
#include <QStringList>

#include "extractprogress.h"

QStringList getIsoContents(const QString &isoPath, QString &errorMessage);

bool extractFileFromIso(const QString &isoPath, const QString &fileName, const QString &outputPath, QString &errorMessage);

bool extractAllFromIso(const QString &isoPath, const QString &outputPath,
                       QString &errorMessage,
                       ExtractProgressCallback progressCallback = nullptr);

bool testLibCdio();

#endif // ISOTOOL_H
