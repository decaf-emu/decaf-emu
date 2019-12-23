#pragma once
#include <QStringList>
#include <QThread>
#include <QWidget>

class SettingsStorage;
class TitleScanner;
class TitleListModel;
class TitleSortFilterProxyModel;

class QStackedLayout;
class QTreeView;
class QListView;

class TitleListWidget : public QWidget
{
   Q_OBJECT

public:
   TitleListWidget(SettingsStorage *settingsStorage, QWidget *parent = nullptr);
   ~TitleListWidget();

   void startTitleScan();

signals:
   void scanDirectoryList(QStringList directories);
   void launchTitle(QString path);
   void statusMessage(QString message, int timeout);

protected slots:
   void settingsChanged();
   void titleScanFinished();

private:
   QStackedLayout *mStackedLayout = nullptr;
   QTreeView *mTitleList = nullptr;
   QListView *mTitleGrid = nullptr;

   SettingsStorage *mSettingsStorage = nullptr;
   QThread mScanThread;
   TitleScanner *mTitleScanner = nullptr;
   TitleListModel *mTitleListModel = nullptr;
   TitleSortFilterProxyModel *mProxyModel = nullptr;

   bool mScanRunning = false;
   bool mScanRequested = false;
   QStringList mCurrentDirectoryList;

   QAction *mCopyPathAction;
};
