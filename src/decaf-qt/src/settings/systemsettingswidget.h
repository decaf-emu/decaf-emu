#pragma once
#include "ui_systemsettings.h"
#include "settingswidget.h"

class SystemSettingsWidget : public SettingsWidget
{
   Q_OBJECT

public:
   SystemSettingsWidget(QWidget *parent = nullptr,
                        Qt::WindowFlags f = Qt::WindowFlags());
   ~SystemSettingsWidget() = default;

   void loadSettings(const Settings &settings) override;
   void saveSettings(Settings &settings) override;

private:
   Ui::SystemSettingsWidget mUi;
};
