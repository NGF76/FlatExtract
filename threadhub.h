// ============================================================
// threadhub.h
// خيط منفصل لاختيار الملفات (يمنع تجميد الواجهة)
// ============================================================

#ifndef THREADHUB_H
#define THREADHUB_H

#include <QObject>
#include <QString>

class ThreadHub : public QObject
{
    Q_OBJECT

public:
    explicit ThreadHub(QObject *parent = nullptr);

public slots:
    void chooseFile();
    void chooseDirectory();

signals:
    void fileSelected(const QString &path);
    void directorySelected(const QString &path);
    void finished();
};

#endif // THREADHUB_H
