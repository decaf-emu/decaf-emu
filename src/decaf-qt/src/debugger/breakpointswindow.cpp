#include "breakpointswindow.h"
#include "ui_breakpointswindow.h"

#include "debugdata.h"
#include "breakpointsmodel.h"

#include <libcpu/cpu_breakpoints.h>
#include <QKeyEvent>

BreakpointsWindow::BreakpointsWindow(QWidget *parent) :
   QWidget(parent),
   ui(new Ui::BreakpointsWindow { })
{
   ui->setupUi(this);
   ui->tableView->installEventFilter(this);
}

BreakpointsWindow::~BreakpointsWindow()
{
   delete ui;
}

void
BreakpointsWindow::setDebugData(DebugData *debugData)
{
   mDebugData = debugData;
   mBreakpointsModel = new BreakpointsModel { this };
   mBreakpointsModel->setDebugData(debugData);

   ui->tableView->setModel(mBreakpointsModel);

   auto textMargin = ui->tableView->style()->pixelMetric(QStyle::PM_FocusFrameHMargin, 0, ui->tableView) + 1;
   ui->tableView->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
   ui->tableView->verticalHeader()->setDefaultSectionSize(fontMetrics().height() + textMargin * 2);

   ui->tableView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
   ui->tableView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
   ui->tableView->update();
}

void
BreakpointsWindow::breakpointsViewDoubleClicked(const QModelIndex &index)
{
   auto address = mBreakpointsModel->data(index, BreakpointsModel::AddressRole);
   if (!address.isValid()) {
      return;
   }

   navigateToTextAddress(address.value<uint32_t>());
}

bool
BreakpointsWindow::eventFilter(QObject *obj, QEvent *event)
{
   if (event->type() == QEvent::KeyPress) {
      QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
      if (keyEvent->key() == Qt::Key_Delete) {
         auto index = ui->tableView->currentIndex();
         if (index.isValid()) {
            auto address = mBreakpointsModel->data(index, BreakpointsModel::AddressRole);
            if (address.isValid()) {
               cpu::removeBreakpoint(address.value<uint32_t>());
               return true;
            }
         }
      }
   }

   return QObject::eventFilter(obj, event);
}
