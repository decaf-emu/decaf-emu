#include "voiceswindow.h"
#include "ui_voiceswindow.h"

#include "debugdata.h"
#include "voicesmodel.h"

VoicesWindow::VoicesWindow(QWidget *parent) :
   QWidget(parent),
   ui(new Ui::VoicesWindow { })
{
   ui->setupUi(this);
}

VoicesWindow::~VoicesWindow()
{
   delete ui;
}

void
VoicesWindow::setDebugData(DebugData *debugData)
{
   mDebugData = debugData;
   mVoicesModel = new VoicesModel { this };
   mVoicesModel->setDebugData(debugData);

   ui->tableView->setModel(mVoicesModel);

   auto textMargin = ui->tableView->style()->pixelMetric(QStyle::PM_FocusFrameHMargin, 0, ui->tableView) + 1;
   ui->tableView->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
   ui->tableView->verticalHeader()->setDefaultSectionSize(fontMetrics().height() + textMargin * 2);

   ui->tableView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
   for (int i = 1; i < ui->tableView->horizontalHeader()->count(); ++i) {
      ui->tableView->horizontalHeader()->setSectionResizeMode(i, QHeaderView::Stretch);
   }
   ui->tableView->update();
}
