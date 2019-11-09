#include "mainwindow.h"

#include "aboutdialog.h"
#include "debugger/debuggerwindow.h"
#include "decafinterface.h"
#include "renderwidget.h"
#include "softwarekeyboarddriver.h"
#include "settings/settingsdialog.h"
#include "titlelistwidget.h"

#include <QCloseEvent>
#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QSettings>
#include <QShortcut>
#include <QTimer>

#include <libdecaf/decaf.h>

MainWindow::MainWindow(SettingsStorage *settingsStorage,
                       DecafInterface *decafInterface,
                       InputDriver *inputDriver,
                       QWidget *parent) :
   QMainWindow(parent),
   mSettingsStorage(settingsStorage),
   mDecafInterface(decafInterface),
   mInputDriver(inputDriver),
   mSoftwareKeyboardDriver(new SoftwareKeyboardDriver { this }),
   mSoftwareKeyboardInputDialog(new QInputDialog { this })
{
   // Setup UI
   mUi.setupUi(this);

   mTitleListWiget = new TitleListWidget { mSettingsStorage, this };
   setCentralWidget(mTitleListWiget);
   connect(mTitleListWiget, &TitleListWidget::launchTitle,
           this, &MainWindow::loadFile);
   connect(mTitleListWiget, &TitleListWidget::statusMessage,
           this, &MainWindow::showStatusMessage);

   connect(decafInterface, &DecafInterface::titleLoaded,
           this, &MainWindow::titleLoaded);
   connect(decafInterface, &DecafInterface::debugInterrupt,
           this, &MainWindow::debugInterrupt);

   // Setup status bar
   mStatusTimer = new QTimer(this);
   mUi.statusBar->addPermanentWidget(mStatusFrameRate = new QLabel());
   mUi.statusBar->addPermanentWidget(mStatusFrameTime = new QLabel());
   connect(mStatusTimer, SIGNAL(timeout()), this, SLOT(updateStatusBar()));

   // Setup settings
   connect(mSettingsStorage, &SettingsStorage::settingsChanged,
           this, &MainWindow::settingsChanged);
   settingsChanged();

   QShortcut *shortcut = new QShortcut(QKeySequence("F11"), this);
   connect(shortcut, &QShortcut::activated,
           this, &MainWindow::toggleFullscreen);
   connect(mSoftwareKeyboardDriver, &SoftwareKeyboardDriver::open,
           this, &MainWindow::softwareKeyboardOpen);
   connect(mSoftwareKeyboardDriver, &SoftwareKeyboardDriver::close,
           this, &MainWindow::softwareKeyboardClose);
   connect(mSoftwareKeyboardDriver, &SoftwareKeyboardDriver::inputStringChanged,
           this, &MainWindow::softwareKeyboardInputStringChanged);
   connect(mSoftwareKeyboardInputDialog, &QInputDialog::finished,
           this, &MainWindow::softwareKeyboardInputFinished);
   decaf::setSoftwareKeyboardDriver(mSoftwareKeyboardDriver);

   // Create recent file actions
   mRecentFilesSeparator = mUi.menuFile->insertSeparator(mUi.actionExit);
   mRecentFilesSeparator->setVisible(false);

   for (auto i = 0u; i < mRecentFileActions.size(); ++i) {
      mRecentFileActions[i] = new QAction(this);
      mRecentFileActions[i]->setVisible(false);
      QObject::connect(mRecentFileActions[i], &QAction::triggered,
                       this, &MainWindow::openRecentFile);
      mUi.menuFile->insertAction(mRecentFilesSeparator, mRecentFileActions[i]);
   }

   updateRecentFileActions();
}

void
MainWindow::softwareKeyboardOpen(QString defaultText)
{
   mSoftwareKeyboardInputDialog->setLabelText("Software Keyboard Input");
   mSoftwareKeyboardInputDialog->show();
}

void
MainWindow::softwareKeyboardClose()
{
   mSoftwareKeyboardInputDialog->close();
}

void
MainWindow::softwareKeyboardInputStringChanged(QString text)
{
   mSoftwareKeyboardInputDialog->setTextValue(text);
}

void
MainWindow::softwareKeyboardInputFinished(int result)
{
   if (result == QDialog::Accepted) {
      mSoftwareKeyboardDriver->acceptInput(mSoftwareKeyboardInputDialog->textValue());
   } else {
      mSoftwareKeyboardDriver->rejectInput();
   }
}

void
MainWindow::settingsChanged()
{
   auto settings = mSettingsStorage->get();
   if (settings->gpu.display.viewMode == gpu::DisplaySettings::Split) {
      mUi.actionViewSplit->setChecked(true);
   } else if (settings->gpu.display.viewMode == gpu::DisplaySettings::TV) {
      mUi.actionViewTV->setChecked(true);
   } else if (settings->gpu.display.viewMode == gpu::DisplaySettings::Gamepad1) {
      mUi.actionViewGamepad1->setChecked(true);
   } else if (settings->gpu.display.viewMode == gpu::DisplaySettings::Gamepad2) {
      mUi.actionViewGamepad2->setChecked(true);
   }

   if (settings->ui.titleListMode == UiSettings::TitleList) {
      mUi.actionViewTitleList->setChecked(true);
   } else if(settings->ui.titleListMode == UiSettings::TitleGrid) {
      mUi.actionViewTitleGrid->setChecked(true);
   }
}

void
MainWindow::titleLoaded(quint64 id, const QString &name)
{
   setWindowTitle(QString("decaf-qt - %1").arg(name));
}

void
MainWindow::debugInterrupt()
{
   openDebugger();
}

bool
MainWindow::loadFile(QString path)
{
   // You only get one chance to run a game out here buddy.
   mUi.actionOpen->setDisabled(true);
   for (auto i = 0u; i < mRecentFileActions.size(); ++i) {
      mRecentFileActions[i]->setDisabled(true);
   }

   // Change main widget to render widget
   mTitleListWiget->deleteLater();
   mTitleListWiget = nullptr;

   mRenderWidget = new RenderWidget { mInputDriver, this };
   setCentralWidget(mRenderWidget);

   // Update status bar
   mUi.statusBar->clearMessage();
   mStatusTimer->start(500);

   // Start the game
   mDecafInterface->startLogging();
   mRenderWidget->startGraphicsDriver();
   mDecafInterface->startGame(path);

   // Update recent files list
   {
      auto settings = QSettings { };
      auto files = settings.value("recentFileList").toStringList();
      files.removeAll(path);
      files.prepend(path);
      while (files.size() > MaxRecentFiles) {
         files.removeLast();
      }

      settings.setValue("recentFileList", files);
   }

   updateRecentFileActions();
   return true;
}

void
MainWindow::openFile()
{
   auto fileName =
      QFileDialog::getOpenFileName(this,
                                   tr("Open Application"), "",
                                   tr("RPX Files (*.rpx);;cos.xml (cos.xml);;"));
   if (!fileName.isEmpty()) {
      loadFile(fileName);
   }
}

void
MainWindow::openRecentFile()
{
   auto action = qobject_cast<QAction *>(sender());
   if (action) {
      loadFile(action->data().toString());
   }
}

void
MainWindow::updateRecentFileActions()
{
   auto settings = QSettings { };
   auto files = settings.value("recentFileList").toStringList();
   auto numRecentFiles = qMin(files.size(), MaxRecentFiles);

   for (int i = 0; i < numRecentFiles; ++i) {
      auto text = QString { "&%1 %2" }.arg(i + 1).arg(QFileInfo { files[i] }.fileName());
      mRecentFileActions[i]->setText(text);
      mRecentFileActions[i]->setData(files[i]);
      mRecentFileActions[i]->setVisible(true);
   }

   for (int j = numRecentFiles; j < MaxRecentFiles; ++j) {
      mRecentFileActions[j]->setVisible(false);
   }

   mRecentFilesSeparator->setVisible(numRecentFiles > 0);
}

void
MainWindow::exit()
{
   close();
}

void
MainWindow::setViewModeSplit()
{
   auto settings = *mSettingsStorage->get();
   settings.gpu.display.viewMode = gpu::DisplaySettings::Split;
   mSettingsStorage->set(settings);
}

void
MainWindow::setViewModeTV()
{
   auto settings = *mSettingsStorage->get();
   settings.gpu.display.viewMode = gpu::DisplaySettings::TV;
   mSettingsStorage->set(settings);
}

void
MainWindow::setViewModeGamepad1()
{
   auto settings = *mSettingsStorage->get();
   settings.gpu.display.viewMode = gpu::DisplaySettings::Gamepad1;
   mSettingsStorage->set(settings);
}

void
MainWindow::setTitleListModeList()
{
   auto settings = *mSettingsStorage->get();
   settings.ui.titleListMode= UiSettings::TitleList;
   mSettingsStorage->set(settings);
}

void
MainWindow::setTitleListModeGrid()
{
   auto settings = *mSettingsStorage->get();
   settings.ui.titleListMode = UiSettings::TitleGrid;
   mSettingsStorage->set(settings);
}

void
MainWindow::toggleFullscreen()
{
   if (mUi.menuBar->isVisible()) {
      mUi.menuBar->hide();
      mUi.statusBar->hide();
      showFullScreen();
   } else {
      mUi.menuBar->show();
      mUi.statusBar->show();
      showNormal();
   }
}

void
MainWindow::openDebugger()
{
   if (mDebuggerWindow) {
      mDebuggerWindow->show();
      return;
   }

   mDebuggerWindow = new DebuggerWindow { };
   mDebuggerWindow->setAttribute(Qt::WA_DeleteOnClose);
   mDebuggerWindow->show();
   connect(mDebuggerWindow, &DebuggerWindow::destroyed, [this]() {
      mDebuggerWindow = nullptr;
   });
}

void
MainWindow::openSettings()
{
   SettingsDialog dialog { mSettingsStorage, mInputDriver, SettingsTab::Default, this };
   dialog.exec();
}

void
MainWindow::openDebugSettings()
{
   SettingsDialog dialog { mSettingsStorage, mInputDriver, SettingsTab::Debug, this };
   dialog.exec();
}

void
MainWindow::openInputSettings()
{
   SettingsDialog dialog { mSettingsStorage, mInputDriver, SettingsTab::Input, this };
   dialog.exec();
}

void
MainWindow::openLoggingSettings()
{
   SettingsDialog dialog { mSettingsStorage, mInputDriver, SettingsTab::Logging, this };
   dialog.exec();
}

void
MainWindow::openSystemSettings()
{
   SettingsDialog dialog { mSettingsStorage, mInputDriver, SettingsTab::System, this };
   dialog.exec();
}

void
MainWindow::openAudioSettings()
{
   SettingsDialog dialog { mSettingsStorage, mInputDriver, SettingsTab::Audio, this };
   dialog.exec();
}

void
MainWindow::openDisplaySettings()
{
   SettingsDialog dialog { mSettingsStorage, mInputDriver, SettingsTab::Display, this };
   dialog.exec();
}

void
MainWindow::openContentSettings()
{
   SettingsDialog dialog { mSettingsStorage, mInputDriver, SettingsTab::Content, this };
   dialog.exec();
}

void
MainWindow::openAboutDialog()
{
   AboutDialog dialog { this };
   dialog.exec();
}

void
MainWindow::closeEvent(QCloseEvent *event)
{
#if 0
   QMessageBox::StandardButton resBtn = QMessageBox::question(this, "Are you sure?",
                                                              tr("A game is running, are you sure you want to exit?\n"),
                                                              QMessageBox::Cancel | QMessageBox::No | QMessageBox::Yes,
                                                              QMessageBox::Yes);
   if (resBtn != QMessageBox::Yes) {
      event->ignore();
      return;
   }
#endif
   if (mDebuggerWindow) {
      mDebuggerWindow->close();
   }

   mDecafInterface->shutdown();
   event->accept();
}

void
MainWindow::updateStatusBar()
{
   if (auto gpuDriver = decaf::getGraphicsDriver()) {
      auto debugInfo = gpuDriver->getDebugInfo();
      mStatusFrameRate->setText(
         QString("FPS: %1")
         .arg(debugInfo->averageFps, 2, 'f', 0));

      mStatusFrameTime->setText(
         QString("Frametime: %1ms")
         .arg(debugInfo->averageFrameTimeMS, 7, 'f', 3));
   } else {
      mStatusFrameRate->setText("");
      mStatusFrameTime->setText("");
   }
}

void
MainWindow::showStatusMessage(QString message, int timeout)
{
   mUi.statusBar->showMessage(message, timeout);
}
