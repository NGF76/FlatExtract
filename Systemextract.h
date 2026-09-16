#ifndef SYSTEMEXTRACT_H
#define SYSTEMEXTRACT_H

#include <QString>


/**
 * @brief extractUsingSystem
 * @param archivePath
 * @param outputFolder
 * @return
 */

bool extractWithSystemTool(const QString &filePath, const QString &outputPath, QString &errorMessage);

QStringList getSystemArchiveContents (const QString &filePath, QString &errorMessage);



#endif // SYSTEMEXTRACT_H
