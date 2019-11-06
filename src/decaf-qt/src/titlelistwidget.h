#pragma once
#include <QThread>
#include <QWidget>

class SettingsStorage;
class TitleScanner;
class TitleListModel;
class QSortFilterProxyModel;

class QStackedLayout;

class QTreeView;
class QListView;


class TitleListWidget : public QWidget
{
   Q_OBJECT

public:
   TitleListWidget(SettingsStorage *settingsStorage, QWidget *parent = nullptr);
   ~TitleListWidget();

protected slots:
   void settingsChanged();

signals:
   void scanDirectoryList(QStringList directories);
   void launchTitle(QString path);

private:
   QStackedLayout *mStackedLayout = nullptr;
   QTreeView *mTitleList = nullptr;
   QListView *mTitleGrid = nullptr;

   SettingsStorage *mSettingsStorage = nullptr;
   QThread mScanThread;
   TitleScanner *mTitleScanner = nullptr;
   TitleListModel *mTitleListModel = nullptr;
   QSortFilterProxyModel *mProxyModel = nullptr;
};
