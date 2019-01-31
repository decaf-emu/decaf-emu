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

class TitleListWorker;
class QMainWindow;

class TitleList : public QWidget
{
   Q_OBJECT

public:
   enum
   {
      COLUMN_NAME,
      COLUMN_VERSION,
      COLUMN_TITLE_ID,
      COLUMN_PATH,
      COLUMN_PATH2,
      COLUMN_COUNT, // Number of columns
   };

   explicit TitleList(QMainWindow *parent, SettingsStorage *settingsStorage);

   void initialiseTreeView();

   ~TitleList() override;

   void populateAsync(const QString &dirPath,
                      bool deepScan);

   void initialiseModel();

   void refreshTitleDirectory();

signals:
   void titleChosen(QString titlePath);
   void shouldCancelWorker();

private:
   void addEntry(const QList<QStandardItem*> &entryItems);
   void validateEntry(const QModelIndex &item);
   void donePopulating(QStringList watchList);

   QMainWindow *mMainWindow = nullptr;
   QVBoxLayout *mLayout = nullptr;
   QTreeView *mTreeView = nullptr;
   QStandardItemModel *mItemModel = nullptr;
   TitleListWorker *mCurrentWorker = nullptr;
   QFileSystemWatcher *mWatcher = nullptr;
   SettingsStorage *mSettingsStorage = nullptr;
};
