#pragma once
#include "settings.h"

#include <QWidget>

class SettingsWidget : public QWidget
{
public:
   SettingsWidget(QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags()) : QWidget(parent, f) {}
   virtual ~SettingsWidget() = default;

   virtual void loadSettings(const Settings &settings) = 0;
   virtual void saveSettings(Settings &settings) = 0;

private:
};
