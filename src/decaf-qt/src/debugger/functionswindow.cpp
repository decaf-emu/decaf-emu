#include "functionswindow.h"
#include "functionsmodel.h"
#include "debugdata.h"

#include "ui_functionswindow.h"

#include <QSortFilterProxyModel>

FunctionsWindow::FunctionsWindow(QWidget *parent) :
   QWidget(parent),
   ui(new Ui::FunctionsWindow { })
{
   ui->setupUi(this);
}

void
FunctionsWindow::setDebugData(DebugData *debugData)
{
   mDebugData = debugData;
   mFunctionsModel = new FunctionsModel { this };
   mFunctionsModel->setDebugData(mDebugData);

   mSortModel = new QSortFilterProxyModel { this };
   mSortModel->setSourceModel(mFunctionsModel);
   mSortModel->setSortCaseSensitivity(Qt::CaseInsensitive);
   mSortModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
   mSortModel->setFilterKeyColumn(0);

   ui->tableView->setModel(mSortModel);

   auto textMargin = ui->tableView->style()->pixelMetric(QStyle::PM_FocusFrameHMargin, 0, ui->tableView) + 1;
   ui->tableView->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
   ui->tableView->verticalHeader()->setDefaultSectionSize(fontMetrics().height() + textMargin * 2);

   ui->tableView->setSortingEnabled(true);
   ui->tableView->update();

   connect(mDebugData, &DebugData::entry, [&]{
      mFunctionsModel->debugDataChanged();
      ui->tableView->sortByColumn(1);
      ui->tableView->resizeColumnToContents(1);
      ui->tableView->resizeColumnToContents(2);
      ui->tableView->resizeColumnToContents(3);
   });
}

void
FunctionsWindow::filterChanged(QString value)
{
   mSortModel->setFilterFixedString(value);
}

void
FunctionsWindow::functionsViewDoubleClicked(const QModelIndex &index)
{
   auto functionIndex = mSortModel->mapToSource(index);
   auto startAddress = mFunctionsModel->data(functionIndex, FunctionsModel::StartAddressRole);
   if (!startAddress.isValid()) {
      return;
   }

   navigateToTextAddress(startAddress.value<uint32_t>());
}
