#ifndef XBOXOG_ISO_H
#define XBOXOG_ISO_H

#include <QString>
#include <QtGlobal>

#include <functional>

struct XboxOgXbeInfo
{
    bool valid = false;
    quint32 baseAddress = 0;
    quint32 sizeOfImage = 0;
    quint32 entryPoint = 0;
    quint32 titleId = 0;
    quint32 region = 0;
    quint32 version = 0;
    QString titleName;
    QString error;
};

using XboxOgProgressCallback = std::function<void(int, int, const QString &)>;

bool isXboxOgIso(const QString &isoPath);
bool readXboxOgXbeInfo(const QString &xbePath, XboxOgXbeInfo &info);
bool extractXboxOgIso(const QString &isoPath, const QString &outputPath,
                      QString &errorMessage,
                      XboxOgProgressCallback progressCallback = nullptr);

#endif
