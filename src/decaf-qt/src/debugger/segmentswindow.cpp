#include "segmentswindow.h"
#include "ui_segmentswindow.h"

#include "debugdata.h"
#include "segmentsmodel.h"

SegmentsWindow::SegmentsWindow(QWidget *parent) :
   QWidget(parent),
   ui(new Ui::SegmentsWindow { })
{
   ui->setupUi(this);
}

SegmentsWindow::~SegmentsWindow()
{
   delete ui;
}

void
SegmentsWindow::setDebugData(DebugData *debugData)
{
   mDebugData = debugData;
   mSegmentsModel = new SegmentsModel { this };
   mSegmentsModel->setDebugData(debugData);

   ui->tableView->setModel(mSegmentsModel);

   auto textMargin = ui->tableView->style()->pixelMetric(QStyle::PM_FocusFrameHMargin, 0, ui->tableView) + 1;
   ui->tableView->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
   ui->tableView->verticalHeader()->setDefaultSectionSize(fontMetrics().height() + textMargin * 2);

   ui->tableView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
   ui->tableView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
   ui->tableView->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
   ui->tableView->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
   ui->tableView->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
   ui->tableView->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
   ui->tableView->horizontalHeader()->setSectionResizeMode(6, QHeaderView::ResizeToContents);
   ui->tableView->update();
}

void
SegmentsWindow::segmentsViewDoubleClicked(const QModelIndex &index)
{
   auto address = mSegmentsModel->data(index, SegmentsModel::AddressRole);
   auto executable = mSegmentsModel->data(index, SegmentsModel::ExecuteRole);
   if (!address.isValid() || !executable.isValid()) {
      return;
   }

   if (executable.toBool()) {
      navigateToTextAddress(address.value<uint32_t>());
   } else {
      navigateToDataAddress(address.value<uint32_t>());
   }
}
