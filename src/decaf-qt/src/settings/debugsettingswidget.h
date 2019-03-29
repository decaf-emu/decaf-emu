#pragma once
#include "ui_debugsettings.h"
#include "settingswidget.h"

class DebugSettingsWidget : public SettingsWidget
{
   Q_OBJECT

public:
   DebugSettingsWidget(QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
   ~DebugSettingsWidget() = default;

   void loadSettings(const Settings &settings) override;
   void saveSettings(Settings &settings) override;

private:
   Ui::DebugSettingsWidget mUi;
};
