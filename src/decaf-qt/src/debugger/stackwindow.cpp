#include "stackwindow.h"
#include "ui_stackwindow.h"

StackWindow::StackWindow(QWidget *parent) :
   QWidget(parent),
   ui(new Ui::StackWindow { })
{
   ui->setupUi(this);
}

void
StackWindow::setDebugData(DebugData *debugData)
{
   mDebugData = debugData;
   ui->stackWidget->setDebugData(mDebugData);
   connect(mDebugData, &DebugData::dataChanged, this, &StackWindow::updateStatus);
   connect(mDebugData, &DebugData::activeThreadIndexChanged, this, &StackWindow::updateStatus);
}

void
StackWindow::updateStatus()
{
   auto start = DebugData::VirtualAddress { 0 };
   auto end = DebugData::VirtualAddress { 0 };
   auto current = DebugData::VirtualAddress { 0 };

   if (auto activeThread = mDebugData->activeThread()) {
      start = activeThread->stackStart;
      end = activeThread->stackEnd;
      current = activeThread->gpr[1];
   }

   ui->labelStatus->setText(
      QString { "Showing stack from %1 to %2. Current %3" }
      .arg(start, 8, 16, QLatin1Char { '0' })
      .arg(end, 8, 16, QLatin1Char { '0' })
      .arg(current, 8, 16, QLatin1Char{ '0' }));
}
