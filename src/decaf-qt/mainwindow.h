#pragma once
#include "ui_mainwindow.h"

#include <array>
#include <QMainWindow>

class DecafInterface;
class SettingsStorage;
class SoftwareKeyboardDriver;
class VulkanWindow;
class InputDriver;

class QAction;
class QInputDialog;
class QLabel;
class QTimer;
class QVulkanInstance;

class MainWindow : public QMainWindow
{
   Q_OBJECT

   static constexpr int MaxRecentFiles = 5;

public:
   explicit MainWindow(SettingsStorage *settingsStorage,
                       QVulkanInstance *vulkanInstance,
                       DecafInterface *decafInterface,
                       InputDriver *inputDriver,
                       QWidget* parent = nullptr);

private slots:
   void openFile();
   void exit();

   void setViewModeGamepad1();
   void setViewModeSplit();
   void setViewModeTV();

   void toggleFullscreen();

   void openSettings();
   void openDebugSettings();
   void openInputSettings();
   void openLoggingSettings();
   void openRecentFile();
   void openSystemSettings();

   void openAboutDialog();

   void titleLoaded(quint64 id, const QString &name);

   void settingsChanged();

   void softwareKeyboardOpen(QString defaultText);
   void softwareKeyboardClose();
   void softwareKeyboardInputStringChanged(QString text);
   void softwareKeyboardInputFinished(int result);

   void updateStatusBar();

protected:
   void closeEvent(QCloseEvent *event) override;

   bool loadFile(QString path);
   void updateRecentFileActions();

private:
   SettingsStorage *mSettingsStorage;
   VulkanWindow *mVulkanWindow;
   DecafInterface *mDecafInterface;
   InputDriver *mInputDriver;
   SoftwareKeyboardDriver *mSoftwareKeyboardDriver;
   QInputDialog *mSoftwareKeyboardInputDialog = nullptr;

   Ui::MainWindow mUi;
   QTimer *mStatusTimer;
   QLabel *mStatusFrameRate;
   QLabel *mStatusFrameTime;

   QAction *mRecentFilesSeparator;
   std::array<QAction *, MaxRecentFiles> mRecentFileActions;
};
