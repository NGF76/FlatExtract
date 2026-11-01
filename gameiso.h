// ============================================================
// gameiso.h
// Detects and extracts PlayStation 2, PlayStation 3, and
// GameCube disc images.
// ============================================================

#ifndef GAMEISO_H
#define GAMEISO_H

#include <QString>
#include <QStringList>

#include "extractprogress.h"

// ============================================================
// Disc image families
// ============================================================

enum class IsoImageFormat {
    Unknown,
    XboxXdvdfs,
    GameCube,
    Iso9660,
    Udf
};

IsoImageFormat detectIsoImageFormat(const QString &isoPath);
QString isoImageFormatName(IsoImageFormat format);
bool hasIso9660FileSystem(const QString &isoPath);

// ============================================================
// ISO 9660 (PlayStation 2)
// ============================================================

QStringList listIso9660Contents(const QString &isoPath, QString &errorMessage);

bool extractIso9660Image(const QString &isoPath, const QString &outputPath,
                         QString &errorMessage,
                         ExtractProgressCallback progressCallback = nullptr);

// ============================================================
// UDF (PlayStation 3)
// ============================================================

QStringList listUdfContents(const QString &isoPath, QString &errorMessage);

bool extractUdfImage(const QString &isoPath, const QString &outputPath,
                     QString &errorMessage,
                     ExtractProgressCallback progressCallback = nullptr);

// ============================================================
// GameCube GCM/FST
// ============================================================

QStringList listGameCubeContents(const QString &isoPath, QString &errorMessage);

bool extractGameCubeImage(const QString &isoPath, const QString &outputPath,
                          QString &errorMessage,
                          ExtractProgressCallback progressCallback = nullptr);

#endif // GAMEISO_H
