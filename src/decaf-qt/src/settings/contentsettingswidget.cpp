#include "contentsettingswidget.h"

#include <QFileDialog>

ContentSettingsWidget::ContentSettingsWidget(QWidget *parent,
                                             Qt::WindowFlags f) :
   SettingsWidget(parent, f)
{
   mUi.setupUi(this);
}

void
ContentSettingsWidget::loadSettings(const Settings &settings)
{
   mUi.lineEditMlcPath->setText(QString::fromStdString(settings.decaf.system.mlc_path));
   mUi.lineEditSlcPath->setText(QString::fromStdString(settings.decaf.system.slc_path));
   mUi.lineEditSdcardPath->setText(QString::fromStdString(settings.decaf.system.sdcard_path));
   mUi.lineEditHfioPath->setText(QString::fromStdString(settings.decaf.system.hfio_path));
   mUi.lineEditResourcesPath->setText(QString::fromStdString(settings.decaf.system.resources_path));
   mUi.lineEditOtpPath->setText(QString::fromStdString(settings.decaf.system.otp_path));

   mUi.listWidgetTitleDirectories->clear();
   for (const auto &path : settings.decaf.system.title_directories) {
      mUi.listWidgetTitleDirectories->addItem(QString::fromStdString(path));
   }

   mUi.lineEditMlcPath->setCursorPosition(0);
   mUi.lineEditSlcPath->setCursorPosition(0);
   mUi.lineEditSdcardPath->setCursorPosition(0);
   mUi.lineEditHfioPath->setCursorPosition(0);
   mUi.lineEditResourcesPath->setCursorPosition(0);
   mUi.lineEditOtpPath->setCursorPosition(0);
   mUi.listWidgetTitleDirectories->setCurrentRow(0);
}

void
ContentSettingsWidget::saveSettings(Settings &settings)
{
   settings.decaf.system.mlc_path = mUi.lineEditMlcPath->text().toStdString();
   settings.decaf.system.slc_path = mUi.lineEditSlcPath->text().toStdString();
   settings.decaf.system.sdcard_path = mUi.lineEditSdcardPath->text().toStdString();
   settings.decaf.system.hfio_path = mUi.lineEditHfioPath->text().toStdString();
   settings.decaf.system.resources_path = mUi.lineEditResourcesPath->text().toStdString();
   settings.decaf.system.otp_path = mUi.lineEditOtpPath->text().toStdString();

   settings.decaf.system.title_directories.clear();
   for (auto i = 0; i < mUi.listWidgetTitleDirectories->count(); ++i) {
      settings.decaf.system.title_directories.emplace_back(
         mUi.listWidgetTitleDirectories->item(i)->text().toStdString());
   }
}

void
ContentSettingsWidget::browseHfioPath()
{
   auto path = QFileDialog::getExistingDirectory(this, tr("Open Directory"),
                                                 mUi.lineEditHfioPath->text());
   if (!path.isEmpty()) {
      mUi.lineEditHfioPath->setText(path);
   }
}

void
ContentSettingsWidget::browseMlcPath()
{
   auto path = QFileDialog::getExistingDirectory(this, tr("Open Directory"),
                                                 mUi.lineEditMlcPath->text());
   if (!path.isEmpty()) {
      mUi.lineEditMlcPath->setText(path);
   }
}

void
ContentSettingsWidget::browseOtpPath()
{

   auto path = QFileDialog::getOpenFileName(this, tr("Open otp.bin"),
                                            mUi.lineEditOtpPath->text());
   if (!path.isEmpty()) {
      mUi.lineEditOtpPath->setText(path);
   }
}

void
ContentSettingsWidget::browseResourcesPath()
{
   auto path = QFileDialog::getExistingDirectory(this, tr("Open Directory"),
                                                 mUi.lineEditResourcesPath->text());
   if (!path.isEmpty()) {
      mUi.lineEditResourcesPath->setText(path);
   }
}

void
ContentSettingsWidget::browseSdcardPath()
{
   auto path = QFileDialog::getExistingDirectory(this, tr("Open Directory"),
                                                 mUi.lineEditSdcardPath->text());
   if (!path.isEmpty()) {
      mUi.lineEditSdcardPath->setText(path);
   }
}

void
ContentSettingsWidget::browseSlcPath()
{
   auto path = QFileDialog::getExistingDirectory(this, tr("Open Directory"),
                                                 mUi.lineEditSlcPath->text());
   if (!path.isEmpty()) {
      mUi.lineEditSlcPath->setText(path);
   }
}

void
ContentSettingsWidget::addTitleDirectory()
{
   auto path = QFileDialog::getExistingDirectory(this, tr("Open Directory"));
   if (!path.isEmpty()) {
      mUi.listWidgetTitleDirectories->addItem(path);
   }
}

void
ContentSettingsWidget::removeTitleDirectory()
{
   qDeleteAll(mUi.listWidgetTitleDirectories->selectedItems());
}
