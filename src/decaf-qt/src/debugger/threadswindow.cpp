#include "threadswindow.h"
#include "ui_threadswindow.h"

#include "debugdata.h"
#include "threadsmodel.h"

ThreadsWindow::ThreadsWindow(QWidget *parent) :
   QWidget(parent),
   ui(new Ui::ThreadsWindow { })
{
   ui->setupUi(this);
}

void
ThreadsWindow::setDebugData(DebugData *debugData)
{
   mDebugData = debugData;
   mThreadsModel = new ThreadsModel { this };
   mThreadsModel->setDebugData(debugData);

   ui->tableView->setModel(mThreadsModel);

   auto textMargin = ui->tableView->style()->pixelMetric(QStyle::PM_FocusFrameHMargin, 0, ui->tableView) + 1;
   ui->tableView->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
   ui->tableView->verticalHeader()->setDefaultSectionSize(fontMetrics().height() + textMargin * 2);

   ui->tableView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
   ui->tableView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
   ui->tableView->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
   ui->tableView->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
   ui->tableView->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
   ui->tableView->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
   ui->tableView->horizontalHeader()->setSectionResizeMode(6, QHeaderView::ResizeToContents);
   ui->tableView->horizontalHeader()->setSectionResizeMode(7, QHeaderView::ResizeToContents);
   ui->tableView->update();
}

void
ThreadsWindow::threadsViewDoubleClicked(const QModelIndex &index)
{
   if (index.isValid()) {
      mDebugData->setActiveThreadIndex(index.row());
   }
}
