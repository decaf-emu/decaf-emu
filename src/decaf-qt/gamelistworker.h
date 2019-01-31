// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <atomic>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>

#include <QList>
#include <QObject>
#include <QRunnable>
#include <QString>

class QStandardItem;

std::u16string UTF8ToUTF16(const std::string &input);

#ifdef _WIN32
std::string UTF16ToUTF8(const std::wstring &input);
std::wstring UTF8ToUTF16W(const std::string &str);
#endif

/**
 * Asynchronous worker object for populating the game list.
 * Communicates with other threads through Qt's signal/slot system.
 */
class TitleListWorker : public QObject, public QRunnable 
{
   Q_OBJECT

public:
   TitleListWorker(QString dirPath, bool deepScan);

   ~TitleListWorker() override;

   /// Starts the processing of directory tree information.
   void run() override;

   /// Tells the worker that it should no longer continue processing. Thread-safe.
   void cancel();

signals:
   /**
    * The `EntryReady` signal is emitted once an entry has been prepared and is ready
    * to be added to the game list.
    * @param entry_items a list with `QStandardItem`s that make up the columns of the new entry.
    */
   void entryReady(QList<QStandardItem*> entryItems);

   /**
    * After the worker has traversed the game directory looking for entries, this signal is emitted
    * with a list of folders that should be watched for changes as well.
    */
   void finished(QStringList watchList);

private:
   void addEntriesToTitleList(const std::string &dirPath, unsigned int recursion = 0);

   QStringList mWatchList;
   QString mDirPath;
   bool mDeepScan;
   std::atomic_bool mStopProcessing;
};
