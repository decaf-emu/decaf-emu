#include "audiosettingswidget.h"

AudioSettingsWidget::AudioSettingsWidget(QWidget *parent, Qt::WindowFlags f) :
   SettingsWidget(parent, f)
{
   mUi.setupUi(this);
}

void
AudioSettingsWidget::loadSettings(const Settings &settings)
{
   mUi.checkBoxPlaybackEnabled->setChecked(settings.sound.playbackEnabled);
}

void
AudioSettingsWidget::saveSettings(Settings &settings)
{
   settings.sound.playbackEnabled = mUi.checkBoxPlaybackEnabled->isChecked();
}
