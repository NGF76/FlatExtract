#ifndef EXTRACTPROGRESS_H
#define EXTRACTPROGRESS_H

#include <QString>

#include <functional>

using ExtractProgressCallback = std::function<void(int current, int total, const QString &currentFile)>;

#endif
