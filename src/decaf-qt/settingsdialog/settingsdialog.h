#pragma once
#include "ui_settings.h"
#include "settingswidget.h"

#include <QDialog>
#include <QVector>

class DecafInterface;
class InputDriver;
class SettingsWidget;
class SettingsStorage;

class QAbstractButton;

enum class SettingsTab
{
   Default = 0,
   System,
   Input,
   Logging,
   Debug,
};

class SettingsDialog : public QDialog
{
   Q_OBJECT

public:
   SettingsDialog(SettingsStorage *settingsStorage,
                  InputDriver *inputDriver,
                  SettingsTab openTab = SettingsTab::Default,
                  QWidget *parent = nullptr,
                  Qt::WindowFlags f = Qt::WindowFlags());
   ~SettingsDialog() = default;

private slots:
   void buttonBoxClicked(QAbstractButton *button);

private:
   Ui::SettingsDialog mUi;
   QVector<SettingsWidget *> mSettings;
   SettingsStorage *mSettingsStorage;
};
