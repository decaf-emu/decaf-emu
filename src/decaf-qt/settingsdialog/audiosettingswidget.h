#pragma once
#include "ui_audiosettings.h"
#include "settingswidget.h"

class AudioSettingsWidget : public SettingsWidget
{
   Q_OBJECT

public:
   AudioSettingsWidget(QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
   ~AudioSettingsWidget() = default;

   void loadSettings(const Settings &settings) override;
   void saveSettings(Settings &settings) override;

private:
   Ui::AudioSettingsWidget mUi;
};
