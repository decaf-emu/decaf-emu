#pragma once
#include "ui_mainwindow.h"

#include <array>
#include <QMainWindow>

class DebuggerWindow;
class DecafInterface;
class InputDriver;
class RenderWidget;
class SettingsStorage;
class SoftwareKeyboardDriver;
class TitleListWidget;

class QAction;
class QInputDialog;
class QLabel;
class QTimer;

class MainWindow : public QMainWindow
{
   Q_OBJECT

   static constexpr int MaxRecentFiles = 5;

public:
   explicit MainWindow(SettingsStorage *settingsStorage,
                       DecafInterface *decafInterface,
                       InputDriver *inputDriver,
                       QWidget* parent = nullptr);

private slots:
   void openFile();
   void exit();

   void setViewModeGamepad1();
   void setViewModeSplit();
   void setViewModeTV();

   void setTitleListModeList();
   void setTitleListModeGrid();

   void toggleFullscreen();

   void openDebugger();

   void openSettings();
   void openDebugSettings();
   void openInputSettings();
   void openLoggingSettings();
   void openRecentFile();
   void openSystemSettings();
   void openAudioSettings();
   void openDisplaySettings();
   void openContentSettings();

   void openAboutDialog();

   void debugInterrupt();
   void titleLoaded(quint64 id, const QString &name);

   void settingsChanged();

   void softwareKeyboardOpen(QString defaultText);
   void softwareKeyboardClose();
   void softwareKeyboardInputStringChanged(QString text);
   void softwareKeyboardInputFinished(int result);

   void updateStatusBar();
   void showStatusMessage(QString message, int timeout);

   bool loadFile(QString path);

protected:
   void closeEvent(QCloseEvent *event) override;
   void updateRecentFileActions();

private:
   SettingsStorage *mSettingsStorage = nullptr;
   RenderWidget *mRenderWidget = nullptr;
   TitleListWidget *mTitleListWiget = nullptr;
   DecafInterface *mDecafInterface = nullptr;
   InputDriver *mInputDriver = nullptr;
   SoftwareKeyboardDriver *mSoftwareKeyboardDriver = nullptr;
   QInputDialog *mSoftwareKeyboardInputDialog = nullptr;
   DebuggerWindow *mDebuggerWindow = nullptr;

   Ui::MainWindow mUi;
   QTimer *mStatusTimer = nullptr;
   QLabel *mStatusFrameRate = nullptr;
   QLabel *mStatusFrameTime = nullptr;

   QAction *mRecentFilesSeparator = nullptr;
   std::array<QAction *, MaxRecentFiles> mRecentFileActions;
};
