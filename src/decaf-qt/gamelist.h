// Copyright 2015 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <QFileSystemWatcher>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QModelIndex>
#include <QSettings>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QString>
#include <QToolButton>
#include <QTreeView>
#include <QVBoxLayout>
#include <QWidget>

#include "settings.h"

class GameListWorker;
class QMainWindow;

enum class GameListOpenTarget 
{
   SaveData,
   ModData,
};

class GameList : public QWidget 
{
   Q_OBJECT

public:
   enum 
   {
      COLUMN_NAME,
      COLUMN_COMPATIBILITY,
      COLUMN_ADD_ONS,
      COLUMN_FILE_TYPE,
      COLUMN_SIZE,
      COLUMN_COUNT, // Number of columns
   };

   explicit GameList(QMainWindow *parent, SettingsStorage *settingsStorage);

   void initialiseTreeView();

   ~GameList() override;

   void populateAsync(const QString &dirPath, 
                      bool deepScan);

   void initialiseModel();

   void refreshGameDirectory();

   static bool GameList::splitPath(const std::string &fullPath, 
                                   std::string *pPath, 
                                   std::string *pFilename,
                                   std::string *pExtension);

signals:
   void gameChosen(QString gamePath);
   void shouldCancelWorker();

private:
   void addEntry(const QList<QStandardItem*> &entryItems);
   void validateEntry(const QModelIndex &item);
   void donePopulating(QStringList watchList);

   QMainWindow *mMainWindow = nullptr;
   QVBoxLayout *mLayout = nullptr;
   QTreeView *mTreeView = nullptr;
   QStandardItemModel *mItemModel = nullptr;
   GameListWorker *mCurrentWorker = nullptr;
   QFileSystemWatcher *mWatcher = nullptr;
   SettingsStorage *mSettingsStorage = nullptr;
};

Q_DECLARE_METATYPE(GameListOpenTarget);
