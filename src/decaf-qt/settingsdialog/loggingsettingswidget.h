#pragma once
#include "ui_loggingsettings.h"
#include "settingswidget.h"

class LoggingSettingsWidget : public SettingsWidget
{
   Q_OBJECT

public:
   LoggingSettingsWidget(QWidget *parent = nullptr,
                         Qt::WindowFlags f = Qt::WindowFlags());
   ~LoggingSettingsWidget() = default;

   void loadSettings(const Settings &settings) override;
   void saveSettings(Settings &settings) override;

private slots:
   void addTraceFilter();
   void removeTraceFilter();
   void browseLogPath();

private:
   Ui::LoggingSettingsWidget mUi;
};
