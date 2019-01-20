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

bool 
GameList::splitPath(const std::string& fullPath, 
                    std::string* pPath, 
                    std::string* pFilename,
                    std::string* pExtension) 
{
   if (fullPath.empty()) {
     return false;
   }

   std::size_t dirEnd = fullPath.find_last_of("/"
     // windows needs the : included for something like just "C:" to be considered a directory
#ifdef _WIN32
     "\\:"
#endif
   );
   if (std::string::npos == dirEnd) {
     dirEnd = 0;
   } else {
     dirEnd += 1;
   }

   std::size_t fnameEnd = fullPath.rfind('.');

   if (fnameEnd < dirEnd || std::string::npos == fnameEnd) {
     fnameEnd = fullPath.size();
   }

   if (pPath) {
     *pPath = fullPath.substr(0, dirEnd);
   }

   if (pFilename) {
     *pFilename = fullPath.substr(dirEnd, fnameEnd - dirEnd);
   }

   if (pExtension) {
     *pExtension = fullPath.substr(fnameEnd);
   }

   return true;
}

GameList::GameList(QMainWindow* parent, 
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

   mItemModel->setHeaderData(COLUMN_NAME, Qt::Horizontal, tr("Name"));
   mItemModel->setHeaderData(COLUMN_COMPATIBILITY, Qt::Horizontal, tr("Compatibility"));  
   mItemModel->setHeaderData(COLUMN_FILE_TYPE - 1, Qt::Horizontal, tr("Title ID"));
   mItemModel->setHeaderData(COLUMN_SIZE - 1, Qt::Horizontal, tr("Path"));

   connect(mTreeView, &QTreeView::activated, this, &GameList::validateEntry);
   connect(mWatcher, &QFileSystemWatcher::directoryChanged, this, &GameList::refreshGameDirectory);

   // We must register all custom types with the Qt Automoc system so that we are able to use it
   // with signals/slots. In this case, QList falls under the umbrells of custom types.
   qRegisterMetaType<QList<QStandardItem*>>("QList<QStandardItem*>");

   mLayout->setContentsMargins(0, 0, 0, 0);
   mLayout->setSpacing(0);
   mLayout->addWidget(mTreeView);
   setLayout(mLayout);
}

GameList::~GameList() 
{
   emit shouldCancelWorker();
}


void 
GameList::addEntry(const QList<QStandardItem*>& entryItems) 
{
   mItemModel->invisibleRootItem()->appendRow(entryItems);
   mTreeView->resizeColumnToContents(0);
}

void 
GameList::validateEntry(const QModelIndex& item) 
{
   // We don't care about the individual QStandardItem that was selected, but its row.
   const int row = mItemModel->itemFromIndex(item)->row();
   const QStandardItem* childFile = mItemModel->invisibleRootItem()->child(row, COLUMN_NAME);
   const QString filePath = childFile->data(GameListItemPath::FullPathRole).toString();

   if (filePath.isEmpty()) {
      return;
   }

   if (!QFileInfo::exists(filePath)) {
      return;
   }

   std::string path = childFile->data(GameListItemPath::FullPathRole).toString().toStdString();

   const QFileInfo file_info{filePath};
   if (file_info.isDir()) {
      const QDir dir{filePath};
      const QStringList matchingMain = dir.entryList(QStringList("main"), QDir::Files);

      if (matchingMain.size() == 1) {
         emit gameChosen(dir.path() + "/" + matchingMain[0]);
      }
      return;
   }

   // Users usually want to run a diffrent game after closing one
   emit gameChosen(filePath);
}

void 
GameList::donePopulating(QStringList watchList) 
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

   mTreeView->setEnabled(true);
}

void
GameList::populateAsync(const QString& dirPath, 
                        bool deepScan) 
{
   const QFileInfo dirInfo{dirPath};

   if (!dirInfo.exists() || !dirInfo.isDir()) {
      return;
   }

   mTreeView->setEnabled(false);

   initialiseModel();

   emit shouldCancelWorker();

   GameListWorker* worker = new GameListWorker(dirPath, deepScan);

   // Use DirectConnection here because worker->Cancel() is thread-safe and we want it to cancel
   // without delay.
   connect(this, &GameList::shouldCancelWorker, worker, &GameListWorker::cancel, Qt::DirectConnection);
   connect(worker, &GameListWorker::entryReady, this, &GameList::addEntry, Qt::QueuedConnection);
   connect(worker, &GameListWorker::finished, this, &GameList::donePopulating, Qt::QueuedConnection);

   QThreadPool::globalInstance()->start(worker);
   mCurrentWorker = std::move(worker);
}

void 
GameList::initialiseTreeView()
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
}

void
GameList::initialiseModel()
{
   // Update the columns in case UISettings has changed
   mItemModel->removeColumns(0, mItemModel->columnCount());

   mItemModel->insertColumns(0, COLUMN_COUNT - 1);

   mItemModel->setHeaderData(COLUMN_NAME, Qt::Horizontal, tr("Name"));
   mItemModel->setHeaderData(COLUMN_COMPATIBILITY, Qt::Horizontal, tr("Compatibility"));
   mItemModel->setHeaderData(COLUMN_FILE_TYPE - 1, Qt::Horizontal, tr("Title ID"));
   mItemModel->setHeaderData(COLUMN_SIZE - 1, Qt::Horizontal, tr("Path"));

   mItemModel->removeColumns(COLUMN_COUNT - 1, 1);

   // Delete any rows that might already exist if we're repopulating
   mItemModel->removeRows(0, mItemModel->rowCount());
}

void 
GameList::refreshGameDirectory() 
{
   auto settings = *mSettingsStorage->get();

   if (!settings.display.gamePath.isEmpty())
   {
      populateAsync(settings.display.gamePath, false);
   }
}
