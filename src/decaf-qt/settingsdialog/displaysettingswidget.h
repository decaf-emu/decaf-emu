#pragma once
#include "ui_displaysettings.h"
#include "settingswidget.h"

class DisplaySettingsWidget : public SettingsWidget
{
   Q_OBJECT

public:
   DisplaySettingsWidget(QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
   ~DisplaySettingsWidget() = default;

   void loadSettings(const Settings &settings) override;
   void saveSettings(Settings &settings) override;

private:
   Ui::DisplaySettingsWidget mUi;
};
