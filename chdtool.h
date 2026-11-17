// ============================================================
// chdtool.h
// CHD (Compressed Hunks of Data) image support.
//
// Converts a CHD disc image to a temporary ISO by calling
// chdman (bundled in the AppImage or provided by mame-tools).
// The resulting ISO is then handled by the regular ISO pipeline.
// ============================================================

#ifndef CHDTOOL_H
#define CHDTOOL_H

#include <QString>

class QWidget;

namespace ChdTool {

// Returns true when the file starts with the CHD magic "MComprHD".
bool isChdFile(const QString &path);

// Converts the CHD image at chdPath into a cached ISO file.
//
// On success the absolute path of the ISO is returned. The conversion is
// cached per file (path + size + mtime), so listing the contents and then
// extracting only performs the (potentially slow) conversion once.
//
// When dialogParent is non-null a modal, cancellable progress dialog is
// shown while chdman runs.
QString convertToIso(const QString &chdPath, QString &errorMessage,
                     QWidget *dialogParent = nullptr);

// Directory that stores the converted ISO files.
QString cacheDirectory();

// Removes every cached ISO file.
void clearCache();

} // namespace ChdTool

#endif // CHDTOOL_H
