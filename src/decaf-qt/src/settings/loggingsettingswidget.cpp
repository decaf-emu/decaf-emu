#include "loggingsettingswidget.h"

#include <QFileDialog>

static const char *LogLevels[] = {
   "trace",
   "debug",
   "info",
   "notice",
   "warning",
   "error",
   "critical",
   "alert",
   "emerg",
   "off"
};

LoggingSettingsWidget::LoggingSettingsWidget(QWidget *parent,
                                             Qt::WindowFlags f) :
   SettingsWidget(parent, f)
{
   mUi.setupUi(this);

   for (auto &level : LogLevels) {
      mUi.comboBoxLogLevel->addItem(level);
   }
}

void
LoggingSettingsWidget::loadSettings(const Settings &settings)
{
   mUi.checkBoxAsynchronous->setChecked(settings.decaf.log.async);
   mUi.checkBoxBranchTracing->setChecked(settings.decaf.log.branch_trace);
   mUi.checkBoxHleTrace->setChecked(settings.decaf.log.hle_trace);
   mUi.checkBoxHleTraceReturnValue->setChecked(settings.decaf.log.hle_trace_res);
   mUi.checkBoxOutputFile->setChecked(settings.decaf.log.to_file);
   mUi.checkBoxOutputStdout->setChecked(settings.decaf.log.to_stdout);
   mUi.lineEditLogDirectory->setText(QString::fromStdString(settings.decaf.log.directory));
   mUi.lineEditLogDirectory->setCursorPosition(0);

   int index = mUi.comboBoxLogLevel->findText(QString::fromStdString(settings.decaf.log.level));
   if (index != -1) {
      mUi.comboBoxLogLevel->setCurrentIndex(index);
   } else {
      mUi.comboBoxLogLevel->setCurrentIndex(1);
   }

   mUi.listWidgetHleTraceFilters->clear();
   for (auto &filter : settings.decaf.log.hle_trace_filters) {
      auto item = new QListWidgetItem(QString::fromStdString(filter));
      item->setFlags(item->flags() | Qt::ItemIsEditable);
      mUi.listWidgetHleTraceFilters->addItem(item);
   }
}

void
LoggingSettingsWidget::saveSettings(Settings &settings)
{
   settings.decaf.log.async = mUi.checkBoxAsynchronous->isChecked();
   settings.decaf.log.branch_trace = mUi.checkBoxBranchTracing->isChecked();
   settings.decaf.log.hle_trace = mUi.checkBoxHleTrace->isChecked();
   settings.decaf.log.hle_trace_res = mUi.checkBoxHleTraceReturnValue->isChecked();
   settings.decaf.log.to_file = mUi.checkBoxOutputFile->isChecked();
   settings.decaf.log.to_stdout = mUi.checkBoxOutputStdout->isChecked();
   settings.decaf.log.directory = mUi.lineEditLogDirectory->text().toStdString();
   settings.decaf.log.level = mUi.comboBoxLogLevel->currentText().toStdString();

   settings.decaf.log.hle_trace_filters.clear();
   for (auto i = 0; i < mUi.listWidgetHleTraceFilters->count(); ++i) {
      settings.decaf.log.hle_trace_filters.emplace_back(mUi.listWidgetHleTraceFilters->item(i)->text().toStdString());
   }
}

void
LoggingSettingsWidget::addTraceFilter()
{
   auto item = new QListWidgetItem(QString::fromStdString("+.*"));
   item->setFlags(item->flags() | Qt::ItemIsEditable);
   mUi.listWidgetHleTraceFilters->addItem(item);
   mUi.listWidgetHleTraceFilters->editItem(item);
}

void
LoggingSettingsWidget::removeTraceFilter()
{
   delete mUi.listWidgetHleTraceFilters->currentItem();
}

void
LoggingSettingsWidget::browseLogPath()
{
   auto path = QFileDialog::getExistingDirectory(this, tr("Open Directory"), mUi.lineEditLogDirectory->text());
   if (!path.isEmpty()) {
      mUi.lineEditLogDirectory->setText(path);
   }
}
