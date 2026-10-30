#ifndef TAREXTRACT_H
#define TAREXTRACT_H

#include <QString>
#include <QStringList>

#include "extractprogress.h"

bool isSupportedTarArchive(const QString &filePath);

QStringList getTarArchiveContents(const QString &filePath, QString &errorMessage);

bool extractTarArchive(const QString &filePath, const QString &outputPath,
                       QString &errorMessage,
                       ExtractProgressCallback progressCallback = nullptr);

#endif
