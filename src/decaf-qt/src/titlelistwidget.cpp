#include "settings.h"
#include "titlelistwidget.h"
#include "titlelistmodel.h"
#include "titlelistscanner.h"
#include "ui_titlelist.h"

#include <libdecaf/decaf_content.h>
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QSortFilterProxyModel>
#include <QStackedLayout>
#include <QTreeView>
#include <QListView>

class TitleSortFilterProxyModel : public QSortFilterProxyModel
{
public:
   TitleSortFilterProxyModel(QObject *parent = nullptr) :
      QSortFilterProxyModel(parent)
   {
   }

   bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override
   {
      auto titleId = sourceModel()->data(
         sourceModel()->index(sourceRow, 0, sourceParent),
         TitleListModel::TitleIdRole).toULongLong();

      if (decaf::isSystemTitle(titleId)) {
         if (!showSystemTitles) {
            return false;
         }
      } else {
         if (!showNonSystemTitles) {
            return false;
         }
      }

      switch (decaf::getTitleTypeFromID(titleId)) {
      case decaf::TitleType::Application:
         return showApplications;
      case decaf::TitleType::Demo:
         return showDemos;
      case decaf::TitleType::Data:
         return showData;
      case decaf::TitleType::DLC:
         return showDLC;
      case decaf::TitleType::Update:
         return showUpdates;
      default:
         return showUnknown;
      }
   }

   bool showApplications = true;
   bool showDemos = true;
   bool showData = false;
   bool showDLC = false;
   bool showUpdates = false;
   bool showNonSystemTitles = true;
   bool showSystemTitles = true;
   bool showUnknown = false;
};

TitleListWidget::TitleListWidget(SettingsStorage *settingsStorage,
                                 QWidget *parent) :
   QWidget(parent),
   mSettingsStorage(settingsStorage),
   mStackedLayout(new QStackedLayout { this })
{
   mStackedLayout->setContentsMargins(0, 0, 0, 0);

   mTitleScanner = new TitleScanner();
   mTitleScanner->moveToThread(&mScanThread);
   mScanThread.start();

   mProxyModel = new TitleSortFilterProxyModel { this };
   mTitleListModel = new TitleListModel { this };
   mProxyModel->setSourceModel(mTitleListModel);

   mTitleList = new QTreeView { };
   mTitleList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
   mTitleList->setSortingEnabled(true);
   mTitleList->setModel(mProxyModel);
   mTitleList->header()->setSortIndicator(0, Qt::AscendingOrder);
   mTitleList->header()->setStretchLastSection(false);
   mTitleList->header()->setSectionResizeMode(QHeaderView::ResizeToContents);

   mTitleGrid = new QListView { };
   mTitleGrid->setViewMode(QListView::IconMode);
   mTitleGrid->setModel(mProxyModel);
   mTitleGrid->setWrapping(true);
   mTitleGrid->setWordWrap(true);
   mTitleGrid->setResizeMode(QListView::Adjust);

   mStackedLayout->addWidget(mTitleList);
   mStackedLayout->addWidget(mTitleGrid);

   mCopyPathAction = new QAction(tr("Copy title path"), this);
   connect(mCopyPathAction, &QAction::triggered, [&]() {
      auto data = QVariant { };
      if (mStackedLayout->currentIndex() == 0) {
         data = mTitleList->model()->data(mTitleList->currentIndex(), TitleListModel::TitlePathRole);
      } else if (mStackedLayout->currentIndex() == 1) {
         data = mTitleGrid->model()->data(mTitleGrid->currentIndex(), TitleListModel::TitlePathRole);
      }

      if (data.isValid()) {
         auto path = data.toString();
         if (!path.isEmpty()) {
            qApp->clipboard()->setText(path);
         }
      }
   });

   for (auto widget : { (QWidget *)mTitleGrid , (QWidget *)mTitleList }) {
      widget->addAction(mCopyPathAction);
      widget->setContextMenuPolicy(Qt::ActionsContextMenu);
   }

   connect(&mScanThread, &QThread::finished, mTitleScanner, &QObject::deleteLater);
   connect(this, &TitleListWidget::scanDirectoryList, mTitleScanner, &TitleScanner::scanDirectoryList);
   connect(mTitleScanner, &TitleScanner::titleFound, mTitleListModel, &TitleListModel::addTitle);
   connect(mTitleScanner, &TitleScanner::scanFinished, this, &TitleListWidget::titleScanFinished);

   connect(mTitleList, &QListView::doubleClicked, [&](const QModelIndex &index) {
      auto data = mTitleList->model()->data(index, TitleListModel::TitlePathRole);
      if (data.isValid()) {
         auto path = data.toString();
         if (!path.isEmpty()) {
            launchTitle(path);
         }
      }
   });

   connect(mTitleGrid, &QListView::doubleClicked, [&](const QModelIndex &index) {
      auto data = mTitleGrid->model()->data(index, TitleListModel::TitlePathRole);
      if (data.isValid()) {
         auto path = data.toString();
         if (!path.isEmpty()) {
            launchTitle(path);
         }
      }
   });

   connect(mSettingsStorage, &SettingsStorage::settingsChanged,
           this, &TitleListWidget::settingsChanged);
   settingsChanged();
}

TitleListWidget::~TitleListWidget()
{
   mScanThread.quit();
   mScanThread.wait();
}

void
TitleListWidget::settingsChanged()
{
   if (mSettingsStorage->get()->ui.titleListMode == UiSettings::TitleGrid) {
      mStackedLayout->setCurrentIndex(1);
   } else {
      mStackedLayout->setCurrentIndex(0);
   }

   startTitleScan();
}

void
TitleListWidget::startTitleScan()
{
   auto settings = mSettingsStorage->get();
   auto directoryList = QStringList { };

   if (!settings->decaf.system.mlc_path.empty()) {
      directoryList.push_back(QString::fromStdString(settings->decaf.system.mlc_path) + "/sys/title");
      directoryList.push_back(QString::fromStdString(settings->decaf.system.mlc_path) + "/usr/title");
   }

   for (const auto &dir : settings->decaf.system.title_directories) {
      directoryList.push_back(QString::fromStdString(dir));
   }

   if (mCurrentDirectoryList == directoryList) {
      return;
   }

   if (mScanRunning) {
      if (!mScanRequested) {
         mTitleScanner->cancel();
         mScanRequested = true;
      }

      return;
   }

   mScanRunning = true;
   mScanRequested = false;
   mCurrentDirectoryList = directoryList;
   mTitleListModel->clear();

   emit statusMessage("Scanning for titles...", 0);
   scanDirectoryList(mCurrentDirectoryList);
}

void
TitleListWidget::titleScanFinished()
{
   mScanRunning = false;

   if (mScanRequested) {
      startTitleScan();
   }

   emit statusMessage(
      QString("Scanning complete, found %1 titles")
         .arg(mTitleListModel->rowCount({})), 0);
}
