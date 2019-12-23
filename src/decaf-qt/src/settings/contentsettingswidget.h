#pragma once
#include "ui_contentsettings.h"
#include "settingswidget.h"

class ContentSettingsWidget : public SettingsWidget
{
   Q_OBJECT

public:
   ContentSettingsWidget(QWidget *parent = nullptr,
                         Qt::WindowFlags f = Qt::WindowFlags());
   ~ContentSettingsWidget() = default;

   void loadSettings(const Settings &settings) override;
   void saveSettings(Settings &settings) override;

private slots:
   void browseHfioPath();
   void browseMlcPath();
   void browseOtpPath();
   void browseResourcesPath();
   void browseSdcardPath();
   void browseSlcPath();
   void addTitleDirectory();
   void removeTitleDirectory();

private:
   Ui::ContentSettingsWidget mUi;
};
