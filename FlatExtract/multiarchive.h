#ifndef MULTIARCHIVE_H
#define MULTIARCHIVE_H

#include <QStringList>

class QListWidget;
class QStatusBar;

class MultiArchive {
public:
 static void  loadMultipleFiles(QListWidget *listWidget, QStatusBar *statusBar);
};

#endif // MULTIARCHIVE_H
