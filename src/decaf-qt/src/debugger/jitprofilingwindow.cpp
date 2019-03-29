#include "jitprofilingwindow.h"
#include "ui_jitprofilingwindow.h"

#include "debugdata.h"
#include "jitprofilingmodel.h"
#include <libcpu/jit_stats.h>

#include <QSortFilterProxyModel>

JitProfilingWindow::JitProfilingWindow(QWidget *parent) :
   QWidget(parent),
   ui(new Ui::JitProfilingWindow { })
{
   ui->setupUi(this);
}

void
JitProfilingWindow::setDebugData(DebugData *debugData)
{
   mDebugData = debugData;
   connect(mDebugData, &DebugData::dataChanged, this, [this]() {
      const auto &stats = mDebugData->jitStats();
      ui->labelJitCodeSize->setText(QString{ "%1 mb" }.arg(stats.usedCodeCacheSize / 1.0e6, 0, 'f', 2));
      ui->labelJitDataSize->setText(QString{ "%1 mb" }.arg(stats.usedDataCacheSize / 1.0e6, 0, 'f', 2));
   });

   mJitProfilingModel = new JitProfilingModel { this };
   mJitProfilingModel->setDebugData(debugData);

   mSortModel = new QSortFilterProxyModel { this };
   mSortModel->setSourceModel(mJitProfilingModel);
   mSortModel->setSortRole(JitProfilingModel::SortRole);

   ui->tableView->setModel(mSortModel);

   auto textMargin = ui->tableView->style()->pixelMetric(QStyle::PM_FocusFrameHMargin, 0, ui->tableView) + 1;
   ui->tableView->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
   ui->tableView->verticalHeader()->setDefaultSectionSize(fontMetrics().height() + textMargin * 2);

   ui->tableView->setSortingEnabled(true);
   ui->tableView->update();

   connect(mDebugData, &DebugData::entry, [&]{
      ui->tableView->sortByColumn(3);
      ui->tableView->resizeColumnToContents(0);
      ui->tableView->resizeColumnToContents(1);
      ui->tableView->resizeColumnToContents(2);
      ui->tableView->resizeColumnToContents(3);
      ui->tableView->resizeColumnToContents(4);
      ui->tableView->resizeColumnToContents(5);
   });
}

void
JitProfilingWindow::clearProfileData()
{
   cpu::jit::resetProfileStats();
}

void
JitProfilingWindow::setProfilingEnabled(bool enabled)
{
   if (enabled) {
      mDebugData->setJitProfilingState(
         ui->checkBoxCore0->isChecked(),
         ui->checkBoxCore1->isChecked(),
         ui->checkBoxCore2->isChecked());
      ui->pushButtonStartStop->setText(tr("Stop"));
   } else {
      mDebugData->setJitProfilingState(false, false, false);
      ui->pushButtonStartStop->setText(tr("Start"));
   }
}

void
JitProfilingWindow::setCore0Mask(bool enabled)
{
   if (ui->pushButtonStartStop->isChecked()) {
      mDebugData->setJitProfilingState(
         ui->checkBoxCore0->isChecked(),
         ui->checkBoxCore1->isChecked(),
         ui->checkBoxCore2->isChecked());
   }
}

void
JitProfilingWindow::setCore1Mask(bool enabled)
{
   if (ui->pushButtonStartStop->isChecked()) {
      mDebugData->setJitProfilingState(
         ui->checkBoxCore0->isChecked(),
         ui->checkBoxCore1->isChecked(),
         ui->checkBoxCore2->isChecked());
   }
}

void
JitProfilingWindow::setCore2Mask(bool enabled)
{
   if (ui->pushButtonStartStop->isChecked()) {
      mDebugData->setJitProfilingState(
         ui->checkBoxCore0->isChecked(),
         ui->checkBoxCore1->isChecked(),
         ui->checkBoxCore2->isChecked());
   }
}

void
JitProfilingWindow::tableViewDoubleClicked(QModelIndex index)
{
   auto profileIndex = mSortModel->mapToSource(index);
   auto startAddress = mJitProfilingModel->data(profileIndex.siblingAtColumn(0), JitProfilingModel::SortRole);
   if (!startAddress.isValid()) {
      return;
   }

   navigateToTextAddress(startAddress.value<uint32_t>());
}
