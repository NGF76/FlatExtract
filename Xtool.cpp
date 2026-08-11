// isobrowser.cpp
#include "Xtool.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QDir>
#include <QFile>
#include <QDebug>

IsoBrowser::IsoBrowser(QWidget* parent) : QMainWindow(parent) {
    auto* central = new QWidget(this);
    auto* layout = new QVBoxLayout(central);

    tree_ = new QTreeWidget(this);
    tree_->setColumnCount(3);
    tree_->setHeaderLabels({"Name", "Size", "Sector"});
    tree_->setColumnWidth(0, 350);

    auto* btnRow = new QHBoxLayout();
    openBtn_ = new QPushButton("Open ISO...", this);
    extractBtn_ = new QPushButton("Extract Selected", this);
    extractBtn_->setEnabled(false);
    btnRow->addWidget(openBtn_);
    btnRow->addWidget(extractBtn_);

    layout->addLayout(btnRow);
    layout->addWidget(tree_);
    setCentralWidget(central);
    resize(700, 500);
    setWindowTitle("XEX/ISO Browser");

    connect(openBtn_, &QPushButton::clicked, this, &IsoBrowser::onOpenIso);
    connect(extractBtn_, &QPushButton::clicked, this, &IsoBrowser::onExtractSelected);
}

void IsoBrowser::onOpenIso() {
    QString path = QFileDialog::getOpenFileName(
        this, "Open Xbox 360 ISO", QString(), "ISO Images (*.iso)");
    if (path.isEmpty()) return;

    if (!iso_.Open(path.toStdString())) {
        QMessageBox::critical(this, "Error",
                              "Failed to open ISO or locate XDVDFS volume.");
        return;
    }

    isoPath_ = path;
    auto files = iso_.ListAllFiles();
    populateTree(files);
    extractBtn_->setEnabled(true);
}

void IsoBrowser::populateTree(const std::vector<IsoFileEntry>& files) {
    tree_->clear();


    for (const auto& f : files) {
        qDebug() << QString::fromStdString(f.path) << (f.isDirectory ? "[DIR]" : "") << f.fileSize;
    }

    // Map directory path -> QTreeWidgetItem, so we can nest children correctly
    QMap<QString, QTreeWidgetItem*> dirItems;
    dirItems[""] = nullptr; // root

    for (const auto& f : files) {
        QString fullPath = QString::fromStdString(f.path);
        int lastSep = fullPath.lastIndexOf('\\');
        QString parentPath = lastSep >= 0 ? fullPath.left(lastSep) : "";
        QString name = lastSep >= 0 ? fullPath.mid(lastSep + 1) : fullPath;

        QTreeWidgetItem* parentItem = dirItems.value(parentPath, nullptr);
        QTreeWidgetItem* item = parentItem
                                    ? new QTreeWidgetItem(parentItem)
                                    : new QTreeWidgetItem(tree_);

        item->setText(0, name);
        item->setText(1, f.isDirectory ? "" : QString::number(f.fileSize));
        item->setText(2, QString::number(f.startSector));
        item->setCheckState(0, Qt::Unchecked);
        item->setData(0, Qt::UserRole, f.fileSize);
        item->setData(0, Qt::UserRole + 1, f.startSector);
        item->setData(0, Qt::UserRole + 2, f.isDirectory);
        item->setData(0, Qt::UserRole + 3, fullPath);

        if (f.isDirectory) {
            dirItems[fullPath] = item;
        }
    }

    //expandToDepth(0);

    tree_->expandAll();
}

void IsoBrowser::onExtractSelected() {
    QString outDir = QFileDialog::getExistingDirectory(this, "Choose output folder");
    if (outDir.isEmpty()) return;

    // Collect checked, non-directory items
    std::vector<QTreeWidgetItem*> toExtract;
    std::function<void(QTreeWidgetItem*)> collect = [&](QTreeWidgetItem* item) {
        for (int i = 0; i < item->childCount(); i++) {
            auto* child = item->child(i);
            bool isDir = child->data(0, Qt::UserRole + 2).toBool();
            if (child->checkState(0) == Qt::Checked && !isDir) {
                toExtract.push_back(child);
            }
            collect(child);
        }
    };
    for (int i = 0; i < tree_->topLevelItemCount(); i++) {
        auto* top = tree_->topLevelItem(i);
        bool isDir = top->data(0, Qt::UserRole + 2).toBool();
        if (top->checkState(0) == Qt::Checked && !isDir) toExtract.push_back(top);
        collect(top);
    }

    if (toExtract.empty()) {
        QMessageBox::information(this, "Nothing selected", "Check at least one file first.");
        return;
    }

    QProgressDialog progress("Extracting...", "Cancel", 0, (int)toExtract.size(), this);
    progress.setWindowModality(Qt::WindowModal);

    int done = 0;
    for (auto* item : toExtract) {
        if (progress.wasCanceled()) break;

        uint32_t sector = (uint32_t)item->data(0, Qt::UserRole + 1).toUInt();
        uint32_t size = (uint32_t)item->data(0, Qt::UserRole).toUInt();
        QString fullPath = item->data(0, Qt::UserRole + 3).toString();

        std::vector<uint8_t> data;
        iso_.ExtractBySector(sector, size, data);

        QString outPath = QDir(outDir).filePath(QFileInfo(fullPath).fileName());
        QFile out(outPath);
        if (out.open(QIODevice::WriteOnly)) {
            out.write((const char*)data.data(), (qint64)data.size());
            out.close();
        }

        // If it's a XEX, parse and show info immediately
        if (fullPath.endsWith(".xex", Qt::CaseInsensitive)) {
            ParsedXex xex;
            if (ParseXex(data, xex)) {
                item->setText(1, QString("%1 (Title 0x%2)")
                                     .arg(size).arg(xex.titleId, 8, 16, QChar('0')));
            }
        }

        progress.setValue(++done);
    }

    QMessageBox::information(this, "Done",
                             QString("Extracted %1 file(s) to %2").arg(done).arg(outDir));
}
