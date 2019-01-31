// Copyright 2015 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <regex>
#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QMenu>
#include <QThreadPool>
#include <fmt/format.h>

#include "gamelist.h"
#include "gamelistp.h"
#include "gamelistworker.h"
#include "qmainwindow.h"

TitleList::TitleList(QMainWindow *parent,
                     SettingsStorage *settingsStorage) :
   QWidget{ parent },
   mSettingsStorage(settingsStorage)
{
   mWatcher = new QFileSystemWatcher(this);
   
   this->mMainWindow = parent;
   mLayout = new QVBoxLayout;
   mTreeView = new QTreeView;
   mItemModel = new QStandardItemModel(mTreeView);

   initialiseTreeView();

   initialiseModel();

   connect(mTreeView, &QTreeView::activated, this, &TitleList::validateEntry);
   connect(mWatcher, &QFileSystemWatcher::directoryChanged, this, &TitleList::refreshTitleDirectory);

   // We must register all custom types with the Qt Automoc system so that we are able to use it
   // with signals/slots. In this case, QList falls under the umbrellas of custom types.
   qRegisterMetaType<QList<QStandardItem*>>("QList<QStandardItem*>");

   mLayout->setContentsMargins(0, 0, 0, 0);
   mLayout->setSpacing(0);
   mLayout->addWidget(mTreeView);
   setLayout(mLayout);
}

TitleList::~TitleList()
{
   emit shouldCancelWorker();
}


void
TitleList::addEntry(const QList<QStandardItem*> &entryItems)
{
   mItemModel->invisibleRootItem()->appendRow(entryItems);
}

void
TitleList::validateEntry(const QModelIndex &item)
{
   // We don't care about the individual QStandardItem that was selected, but its row.
   const int row = mItemModel->itemFromIndex(item)->row();
   const QStandardItem *childFile = mItemModel->invisibleRootItem()->child(row, COLUMN_NAME);
   const QString filePath = childFile->data(TitleListItemPath::FullPathRole).toString();

   if (filePath.isEmpty()) {
      return;
   }

   if (!QFileInfo::exists(filePath)) {
      return;
   }

   std::string path = childFile->data(TitleListItemPath::FullPathRole).toString().toStdString();

   const QFileInfo file_info{filePath};
   if (file_info.isDir()) {
      const QDir dir{filePath};
      const QStringList matchingMain = dir.entryList(QStringList("main"), QDir::Files);

      if (matchingMain.size() == 1) {
         emit titleChosen(dir.path() + "/" + matchingMain[0]);
      }
      return;
   }

   // Users usually want to run a different title after closing one
   emit titleChosen(filePath);
}

void
TitleList::donePopulating(QStringList watchList)
{
   // Clear out the old directories to watch for changes and add the new ones
   auto watchDirs = mWatcher->directories();

   if (!watchDirs.isEmpty()) {
      mWatcher->removePaths(watchDirs);
   }

   // Workaround: Add the watch paths in chunks to allow the gui to refresh
   // This prevents the UI from stalling when a large number of watch paths are added
   // Also artificially caps the watcher to a certain number of directories
   constexpr int LIMIT_WATCH_DIRECTORIES = 5000;
   constexpr int SLICE_SIZE = 25;
   int len = std::min(watchList.length(), LIMIT_WATCH_DIRECTORIES);

   for (int i = 0; i < len; i += SLICE_SIZE) {
      mWatcher->addPaths(watchList.mid(i, i + SLICE_SIZE));
      QCoreApplication::processEvents();
   }

   for (int i = 0; i < COLUMN_PATH; ++i) {
      mTreeView->resizeColumnToContents(i);
   }

   mTreeView->setEnabled(true);
}

void
TitleList::populateAsync(const QString &dirPath,
                        bool deepScan)
{
   const QFileInfo dirInfo{dirPath};

   if (!dirInfo.exists() || !dirInfo.isDir()) {
      return;
   }

   mTreeView->setEnabled(false);

   initialiseModel();

   emit shouldCancelWorker();

   TitleListWorker *worker = new TitleListWorker(dirPath, deepScan);

   // Use DirectConnection here because worker->Cancel() is thread-safe and we want it to cancel without delay.
   connect(this, &TitleList::shouldCancelWorker, worker, &TitleListWorker::cancel, Qt::DirectConnection);
   connect(worker, &TitleListWorker::entryReady, this, &TitleList::addEntry, Qt::QueuedConnection);
   connect(worker, &TitleListWorker::finished, this, &TitleList::donePopulating, Qt::QueuedConnection);

   QThreadPool::globalInstance()->start(worker);
   mCurrentWorker = std::move(worker);
}

void
TitleList::initialiseTreeView()
{
   mTreeView->setModel(mItemModel);
   mTreeView->setWordWrap(false);
   mTreeView->setAlternatingRowColors(true);
   mTreeView->setSelectionMode(QHeaderView::SingleSelection);
   mTreeView->setSelectionBehavior(QHeaderView::SelectRows);
   mTreeView->setVerticalScrollMode(QHeaderView::ScrollPerPixel);
   mTreeView->setHorizontalScrollMode(QHeaderView::ScrollPerPixel);
   mTreeView->setSortingEnabled(true);
   mTreeView->setEditTriggers(QHeaderView::NoEditTriggers);
   mTreeView->setUniformRowHeights(true);
   mTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
   mTreeView->setStyleSheet("QTreeView{ border: none; }");

   mItemModel->insertColumns(0, COLUMN_COUNT - 1);
   mTreeView->setColumnWidth(0, 400);
   mTreeView->setColumnWidth(1, 600);
   mTreeView->setColumnWidth(2, 800);
}

void
TitleList::initialiseModel()
{
   // Update the columns in case UISettings has changed
   mItemModel->removeColumns(0, mItemModel->columnCount());

   mItemModel->insertColumns(0, COLUMN_COUNT - 1);

   mItemModel->setHeaderData(COLUMN_NAME, Qt::Horizontal, tr("Name"));
   mItemModel->setHeaderData(COLUMN_VERSION, Qt::Horizontal, tr("Version"));
   mItemModel->setHeaderData(COLUMN_TITLE_ID, Qt::Horizontal, tr("Title ID"));
   mItemModel->setHeaderData(COLUMN_PATH, Qt::Horizontal, tr("Path"));

   // Delete any rows that might already exist if we're repopulating
   mItemModel->removeRows(0, mItemModel->rowCount());
}

void
TitleList::refreshTitleDirectory()
{
   auto settings = *mSettingsStorage->get();

   if (!settings.display.titlePath.isEmpty()) {
      populateAsync(settings.display.titlePath, false);
   }
}
