#include "displaysettingswidget.h"

#include <QDoubleValidator>

DisplaySettingsWidget::DisplaySettingsWidget(QWidget *parent,
                                             Qt::WindowFlags f) :
   SettingsWidget(parent, f)
{
   mUi.setupUi(this);

   mUi.comboBoxViewMode->addItem(tr("Split"), static_cast<int>(DisplaySettings::Split));
   mUi.comboBoxViewMode->addItem(tr("TV"), static_cast<int>(DisplaySettings::TV));
   mUi.comboBoxViewMode->addItem(tr("Gamepad 1"), static_cast<int>(DisplaySettings::Gamepad1));
   mUi.comboBoxViewMode->addItem(tr("Gamepad 2"), static_cast<int>(DisplaySettings::Gamepad2));

   mUi.lineEditSplitSeparation->setValidator(new QDoubleValidator { });
}

void
DisplaySettingsWidget::loadSettings(const Settings &settings)
{
   auto index = mUi.comboBoxViewMode->findData(static_cast<int>(settings.display.viewMode));
   if (index != -1) {
      mUi.comboBoxViewMode->setCurrentIndex(index);
   } else {
      mUi.comboBoxViewMode->setCurrentIndex(2);
   }

   mUi.checkBoxMaintainAspectRatio->setChecked(settings.display.maintainAspectRatio);
   mUi.lineEditSplitSeparation->setText(QString("%1").arg(settings.display.splitSeperation, 0, 'g', 2));
   mUi.lineEditBackgroundColour->setColour(settings.display.backgroundColour);
}

void
DisplaySettingsWidget::saveSettings(Settings &settings)
{
   settings.display.viewMode = static_cast<DisplaySettings::ViewMode>(mUi.comboBoxViewMode->currentData().toInt());
   settings.display.maintainAspectRatio = mUi.checkBoxMaintainAspectRatio->isChecked();
   settings.display.splitSeperation = mUi.lineEditSplitSeparation->text().toDouble();
   settings.display.backgroundColour = mUi.lineEditBackgroundColour->getColour();
}
