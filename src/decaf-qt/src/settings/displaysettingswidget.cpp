#include "displaysettingswidget.h"

#include <QDoubleValidator>

DisplaySettingsWidget::DisplaySettingsWidget(QWidget *parent,
                                             Qt::WindowFlags f) :
   SettingsWidget(parent, f)
{
   mUi.setupUi(this);

   mUi.comboBoxViewMode->addItem(tr("Split"), static_cast<int>(gpu::DisplaySettings::Split));
   mUi.comboBoxViewMode->addItem(tr("TV"), static_cast<int>(gpu::DisplaySettings::TV));
   mUi.comboBoxViewMode->addItem(tr("Gamepad 1"), static_cast<int>(gpu::DisplaySettings::Gamepad1));
   mUi.comboBoxViewMode->addItem(tr("Gamepad 2"), static_cast<int>(gpu::DisplaySettings::Gamepad2));

   mUi.lineEditSplitSeparation->setValidator(new QDoubleValidator { });
}

void
DisplaySettingsWidget::loadSettings(const Settings &settings)
{
   auto index = mUi.comboBoxViewMode->findData(static_cast<int>(settings.gpu.display.viewMode));
   if (index != -1) {
      mUi.comboBoxViewMode->setCurrentIndex(index);
   } else {
      mUi.comboBoxViewMode->setCurrentIndex(2);
   }

   mUi.checkBoxMaintainAspectRatio->setChecked(
      settings.gpu.display.maintainAspectRatio);

   mUi.lineEditSplitSeparation->setText(
      QString { "%1" }.arg(settings.gpu.display.splitSeperation, 0, 'g', 2));

   mUi.lineEditBackgroundColour->setColour(QColor {
      settings.gpu.display.backgroundColour[0],
      settings.gpu.display.backgroundColour[1],
      settings.gpu.display.backgroundColour[2],
   });
}

void
DisplaySettingsWidget::saveSettings(Settings &settings)
{
   settings.gpu.display.viewMode =
      static_cast<gpu::DisplaySettings::ViewMode>(mUi.comboBoxViewMode->currentData().toInt());

   settings.gpu.display.maintainAspectRatio =
      mUi.checkBoxMaintainAspectRatio->isChecked();

   settings.gpu.display.splitSeperation =
      mUi.lineEditSplitSeparation->text().toDouble();

   auto colour = mUi.lineEditBackgroundColour->getColour();
   settings.gpu.display.backgroundColour[0] = colour.red();
   settings.gpu.display.backgroundColour[1] = colour.green();
   settings.gpu.display.backgroundColour[2] = colour.blue();
}
