#ifndef XTOOL_H
#define XTOOL_H

// Xtool.h
#pragma once
#include <QMainWindow>
#include <QTreeWidget>
#include <QPushButton>
#include <QProgressDialog>
#include "iso_reader.h" // your IsoReader / ParseXex from before
#include "iso_reader.h"
#include "xex_parser.h"

class IsoBrowser : public QMainWindow {
    Q_OBJECT
public:
    explicit IsoBrowser(QWidget* parent = nullptr);

private slots:
    void onOpenIso();
    void onExtractSelected();

private:
    void populateTree(const std::vector<IsoFileEntry>& files);

    QTreeWidget* tree_;
    QPushButton* openBtn_;
    QPushButton* extractBtn_;
    IsoReader iso_;
    QString isoPath_;
};


#endif // XTOOL_H
