#include "systemsettingswidget.h"

#include <QFileDialog>

static constexpr const char *LibraryList[] = {
   "avm.rpl",
   "camera.rpl",
   "coreinit.rpl",
   "dc.rpl",
   "dmae.rpl",
   "drmapp.rpl",
   "erreula.rpl",
   "gx2.rpl",
   "h264.rpl",
   "lzma920.rpl",
   "mic.rpl",
   "nfc.rpl",
   "nio_prof.rpl",
   "nlibcurl.rpl",
   "nlibnss2.rpl",
   "nlibnss.rpl",
   "nn_ac.rpl",
   "nn_acp.rpl",
   "nn_act.rpl",
   "nn_aoc.rpl",
   "nn_boss.rpl",
   "nn_ccr.rpl",
   "nn_cmpt.rpl",
   "nn_dlp.rpl",
   "nn_ec.rpl",
   "nn_fp.rpl",
   "nn_hai.rpl",
   "nn_hpad.rpl",
   "nn_idbe.rpl",
   "nn_ndm.rpl",
   "nn_nets2.rpl",
   "nn_nfp.rpl",
   "nn_nim.rpl",
   "nn_olv.rpl",
   "nn_pdm.rpl",
   "nn_save.rpl",
   "nn_sl.rpl",
   "nn_spm.rpl",
   "nn_temp.rpl",
   "nn_uds.rpl",
   "nn_vctl.rpl",
   "nsysccr.rpl",
   "nsyshid.rpl",
   "nsyskbd.rpl",
   "nsysnet.rpl",
   "nsysuhs.rpl",
   "nsysuvd.rpl",
   "ntag.rpl",
   "padscore.rpl",
   "proc_ui.rpl",
   "sndcore2.rpl",
   "snd_core.rpl",
   "snduser2.rpl",
   "snd_user.rpl",
   "swkbd.rpl",
   "sysapp.rpl",
   "tcl.rpl",
   "tve.rpl",
   "uac.rpl",
   "uac_rpl.rpl",
   "usb_mic.rpl",
   "uvc.rpl",
   "uvd.rpl",
   "vpadbase.rpl",
   "vpad.rpl",
   "zlib125.rpl",
};

static constexpr const char *DisabledLibraryList[] = {
   "coreinit.rpl",
   "gx2.rpl",
   "tcl.rpl",
};

using SystemRegion = decaf::SystemRegion;

SystemSettingsWidget::SystemSettingsWidget(QWidget *parent,
                                           Qt::WindowFlags f) :
   SettingsWidget(parent, f)
{
   mUi.setupUi(this);

   for (auto library : LibraryList) {
      auto item = new QListWidgetItem(library);
      item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
      item->setCheckState(Qt::Unchecked);

      if (std::find(std::begin(DisabledLibraryList), std::end(DisabledLibraryList), library) != std::end(DisabledLibraryList)) {
         item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
      }

      mUi.listWidgetLleWhitelist->addItem(item);
   }

   mUi.comboBoxRegion->addItem(tr("Japan"), static_cast<int>(SystemRegion::Japan));
   mUi.comboBoxRegion->addItem(tr("USA"), static_cast<int>(SystemRegion::USA));
   mUi.comboBoxRegion->addItem(tr("Europe"), static_cast<int>(SystemRegion::Europe));
   mUi.comboBoxRegion->addItem(tr("Unknown8"), static_cast<int>(SystemRegion::Unknown8));
   mUi.comboBoxRegion->addItem(tr("China"), static_cast<int>(SystemRegion::China));
   mUi.comboBoxRegion->addItem(tr("Korea"), static_cast<int>(SystemRegion::Korea));
   mUi.comboBoxRegion->addItem(tr("Taiwan"), static_cast<int>(SystemRegion::Taiwan));

   mUi.lineEditTimeScale->setValidator(new QDoubleValidator(0.01, 100, 2, this));
}

void
SystemSettingsWidget::loadSettings(const Settings &settings)
{
   mUi.lineEditContentPath->setText(QString::fromStdString(settings.decaf.system.content_path));
   mUi.lineEditMlcPath->setText(QString::fromStdString(settings.decaf.system.mlc_path));
   mUi.lineEditSlcPath->setText(QString::fromStdString(settings.decaf.system.slc_path));
   mUi.lineEditSdcardPath->setText(QString::fromStdString(settings.decaf.system.sdcard_path));
   mUi.lineEditHfioPath->setText(QString::fromStdString(settings.decaf.system.hfio_path));
   mUi.lineEditResourcesPath->setText(QString::fromStdString(settings.decaf.system.resources_path));
   mUi.lineEditTimeScale->setText(QString("%1").arg(settings.decaf.system.time_scale, 0, 'g', 2));

   mUi.lineEditContentPath->setCursorPosition(0);
   mUi.lineEditMlcPath->setCursorPosition(0);
   mUi.lineEditSlcPath->setCursorPosition(0);
   mUi.lineEditSdcardPath->setCursorPosition(0);
   mUi.lineEditHfioPath->setCursorPosition(0);
   mUi.lineEditResourcesPath->setCursorPosition(0);

   int index = mUi.comboBoxRegion->findData(static_cast<int>(settings.decaf.system.region));
   if (index != -1) {
      mUi.comboBoxRegion->setCurrentIndex(index);
   } else {
      mUi.comboBoxRegion->setCurrentIndex(2);
   }

   for (auto i = 0; i < mUi.listWidgetLleWhitelist->count(); ++i) {
      auto item = mUi.listWidgetLleWhitelist->item(i);
      auto name = item->text().toStdString();

      if (std::find(settings.decaf.system.lle_modules.begin(), settings.decaf.system.lle_modules.end(), name) != settings.decaf.system.lle_modules.end()) {
         item->setCheckState(Qt::Checked);
      } else {
         item->setCheckState(Qt::Unchecked);
      }
   }
}

void
SystemSettingsWidget::saveSettings(Settings &settings)
{
   settings.decaf.system.content_path = mUi.lineEditContentPath->text().toStdString();
   settings.decaf.system.mlc_path = mUi.lineEditMlcPath->text().toStdString();
   settings.decaf.system.slc_path = mUi.lineEditSlcPath->text().toStdString();
   settings.decaf.system.sdcard_path = mUi.lineEditSdcardPath->text().toStdString();
   settings.decaf.system.hfio_path = mUi.lineEditHfioPath->text().toStdString();
   settings.decaf.system.resources_path = mUi.lineEditResourcesPath->text().toStdString();
   settings.decaf.system.time_scale = mUi.lineEditTimeScale->text().toDouble();
   settings.decaf.system.region = static_cast<SystemRegion>(mUi.comboBoxRegion->currentData().toInt());

   settings.decaf.system.lle_modules.clear();
   for (auto i = 0; i < mUi.listWidgetLleWhitelist->count(); ++i) {
      auto item = mUi.listWidgetLleWhitelist->item(i);
      if (item->checkState() == Qt::Checked) {
         settings.decaf.system.lle_modules.emplace_back(item->text().toStdString());
      }
   }
}

void
SystemSettingsWidget::browseContentPath()
{
   auto path = QFileDialog::getExistingDirectory(this, tr("Open Directory"), mUi.lineEditContentPath->text());
   if (!path.isEmpty()) {
      mUi.lineEditContentPath->setText(path);
   }
}

void
SystemSettingsWidget::browseHfioPath()
{
   auto path = QFileDialog::getExistingDirectory(this, tr("Open Directory"), mUi.lineEditHfioPath->text());
   if (!path.isEmpty()) {
      mUi.lineEditHfioPath->setText(path);
   }
}

void
SystemSettingsWidget::browseMlcPath()
{
   auto path = QFileDialog::getExistingDirectory(this, tr("Open Directory"), mUi.lineEditMlcPath->text());
   if (!path.isEmpty()) {
      mUi.lineEditMlcPath->setText(path);
   }
}

void
SystemSettingsWidget::browseResourcesPath()
{
   auto path = QFileDialog::getExistingDirectory(this, tr("Open Directory"), mUi.lineEditResourcesPath->text());
   if (!path.isEmpty()) {
      mUi.lineEditResourcesPath->setText(path);
   }
}

void
SystemSettingsWidget::browseSdcardPath()
{
   auto path = QFileDialog::getExistingDirectory(this, tr("Open Directory"), mUi.lineEditSdcardPath->text());
   if (!path.isEmpty()) {
      mUi.lineEditSdcardPath->setText(path);
   }
}

void
SystemSettingsWidget::browseSlcPath()
{
   auto path = QFileDialog::getExistingDirectory(this, tr("Open Directory"), mUi.lineEditSlcPath->text());
   if (!path.isEmpty()) {
      mUi.lineEditSlcPath->setText(path);
   }
}
