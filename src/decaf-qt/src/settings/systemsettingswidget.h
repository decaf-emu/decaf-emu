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

private slots:
   void browseContentPath();
   void browseHfioPath();
   void browseMlcPath();
   void browseResourcesPath();
   void browseSdcardPath();
   void browseSlcPath();

private:
   Ui::SystemSettingsWidget mUi;
};
