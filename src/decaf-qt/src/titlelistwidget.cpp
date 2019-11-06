#pragma optimize("", off)
#include "titlelistwidget.h"
#include "titlelistmodel.h"
#include "titlelistscanner.h"

#include "settings.h"
#include "ui_titlelist.h"

#include <QSortFilterProxyModel>
#include <QStackedLayout>
#include <QTreeView>
#include <QListView>

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

   mProxyModel = new QSortFilterProxyModel { this };
   mTitleListModel = new TitleListModel { this };
   mProxyModel->setSourceModel(mTitleListModel);
   mProxyModel->setFilterRole(TitleListModel::TitleTypeRole);
   mProxyModel->setFilterFixedString(tr("Title"));

   mTitleList = new QTreeView { };
   mTitleList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
   mTitleList->setSortingEnabled(true);
   mTitleList->setModel(mProxyModel);
   mTitleList->setColumnHidden(3, true);
   mTitleList->header()->setSortIndicator(0, Qt::AscendingOrder);
   mTitleList->header()->setStretchLastSection(false);
   mTitleList->header()->setSectionResizeMode(QHeaderView::ResizeToContents);

   mTitleGrid = new QListView{ };
   mTitleGrid->setViewMode(QListView::IconMode);
   mTitleGrid->setModel(mProxyModel);
   mTitleGrid->setWrapping(true);
   mTitleGrid->setWordWrap(true);
   mTitleGrid->setResizeMode(QListView::Adjust);

   mStackedLayout->addWidget(mTitleList);
   mStackedLayout->addWidget(mTitleGrid);

   connect(&mScanThread, &QThread::finished, mTitleScanner, &QObject::deleteLater);
   connect(this, &TitleListWidget::scanDirectoryList, mTitleScanner, &TitleScanner::scanDirectoryList);
   connect(mTitleScanner, &TitleScanner::titleFound, mTitleListModel, &TitleListModel::addTitle);

   // Start scanning
   auto directoryList = QStringList{ };
   for (const auto &dir : settingsStorage->get()->decaf.system.title_directories) {
      directoryList.push_back(QString::fromStdString(dir));
   }
   scanDirectoryList(directoryList);

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
}
