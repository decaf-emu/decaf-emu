#include "settingsdialog.h"
#include "settings.h"

#include "audiosettingswidget.h"
#include "debugsettingswidget.h"
#include "displaysettingswidget.h"
#include "inputsettingswidget.h"
#include "loggingsettingswidget.h"
#include "systemsettingswidget.h"

SettingsDialog::SettingsDialog(SettingsStorage *settingsStorage,
                               InputDriver *inputDriver,
                               SettingsTab tab,
                               QWidget *parent,
                               Qt::WindowFlags f) :
   QDialog(parent, f),
   mSettingsStorage(settingsStorage)
{
   mUi.setupUi(this);

   auto inputSettings = new InputSettingsWidget { inputDriver };
   mUi.tabWidget->addTab(inputSettings, tr("Input"));
   mSettings.push_back(inputSettings);

   auto systemSettings = new SystemSettingsWidget { };
   mUi.tabWidget->addTab(systemSettings, tr("System"));
   mSettings.push_back(systemSettings);

   auto displaySettings = new DisplaySettingsWidget { };
   mUi.tabWidget->addTab(displaySettings, tr("Display"));
   mSettings.push_back(displaySettings);

   auto audioSettingsWidget = new AudioSettingsWidget { };
   mUi.tabWidget->addTab(audioSettingsWidget, tr("Audio"));
   mSettings.push_back(audioSettingsWidget);

   auto loggingSettings = new LoggingSettingsWidget { };
   mUi.tabWidget->addTab(loggingSettings, tr("Logging"));
   mSettings.push_back(loggingSettings);

   auto debugSettings = new DebugSettingsWidget { };
   mUi.tabWidget->addTab(debugSettings, tr("Debug"));
   mSettings.push_back(debugSettings);

   // Load all the settings
   auto settings = mSettingsStorage->get();
   for (auto &settingsWidget : mSettings) {
      settingsWidget->loadSettings(*settings);
   }

   if (tab == SettingsTab::Default || tab == SettingsTab::Input) {
      mUi.tabWidget->setCurrentWidget(inputSettings);
   } else if (tab == SettingsTab::System) {
      mUi.tabWidget->setCurrentWidget(systemSettings);
   } else if (tab == SettingsTab::Logging) {
      mUi.tabWidget->setCurrentWidget(loggingSettings);
   } else if (tab == SettingsTab::Debug) {
      mUi.tabWidget->setCurrentWidget(debugSettings);
   }
}

void
SettingsDialog::buttonBoxClicked(QAbstractButton *button)
{
   auto role = mUi.buttonBox->buttonRole(button);
   auto settings = mSettingsStorage->get();

   // On Reset we should load settings
   if (role == QDialogButtonBox::ResetRole) {
      for (auto &settingsWidget : mSettings) {
         settingsWidget->loadSettings(*settings);
      }
   }

   // On Accept or Apply we should save settings
   if (role == QDialogButtonBox::AcceptRole ||
       role == QDialogButtonBox::ApplyRole) {
      // Update config with settings
      auto updatedSettings = *settings;
      for (auto &settingsWidget : mSettings) {
         settingsWidget->saveSettings(updatedSettings);
      }

      mSettingsStorage->set(updatedSettings);
   }

   if (role == QDialogButtonBox::AcceptRole) {
      accept();
   } else if (role == QDialogButtonBox::RejectRole) {
      reject();
   }
}
