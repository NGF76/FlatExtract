#ifndef TOOL_H
#define TOOL_H
#include <QString>
#include <QStringList>

//================================================
//function of Serch file 7z gz tar zip or any file
//================================================
QString findArchiveExecutable();

//=========================
//function of Extract List
//=========================
QStringList getArchiveContents(const QString &filePath, QString &errorMessage);

//=============================
//function of Extract to Folder
//=============================
bool extractArchive(const QString &filePath, const QString &outputPath, QString &errorMessage);



#endif // TOOL_H
