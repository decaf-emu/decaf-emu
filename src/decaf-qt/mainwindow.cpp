#include "mainwindow.h"
#include "aboutdialog.h"
#include "vulkanwindow.h"
#include "softwarekeyboarddriver.h"
#include "settingsdialog/settingsdialog.h"

#include <QCloseEvent>
#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QSettings>
#include <QShortcut>
#include <QTimer>

#include <libdecaf/decaf.h>

MainWindow::MainWindow(SettingsStorage *settingsStorage,
                       QVulkanInstance *vulkanInstance,
                       DecafInterface *decafInterface,
                       InputDriver *inputDriver,
                       QWidget* parent) :
   QMainWindow(parent),
   mSettingsStorage(settingsStorage),
   mDecafInterface(decafInterface),
   mInputDriver(inputDriver),
   mSoftwareKeyboardDriver(new SoftwareKeyboardDriver(this)),
   mSoftwareKeyboardInputDialog(new QInputDialog(this))
{
   // Setup UI
   mUi.setupUi(this);

   mVulkanWindow = new VulkanWindow { vulkanInstance, settingsStorage, decafInterface, inputDriver };
   auto wrapper = QWidget::createWindowContainer(static_cast<QWindow *>(mVulkanWindow));
   wrapper->setFocusPolicy(Qt::StrongFocus);
   wrapper->setFocus();
   setCentralWidget(QWidget::createWindowContainer(mVulkanWindow));

   connect(decafInterface, &DecafInterface::titleLoaded,
           this, &MainWindow::titleLoaded);

   // Setup status bar
   mStatusTimer = new QTimer(this);
   mUi.statusBar->addPermanentWidget(mStatusFrameRate = new QLabel());
   mUi.statusBar->addPermanentWidget(mStatusFrameTime = new QLabel());
   connect(mStatusTimer, SIGNAL(timeout()), this, SLOT(updateStatusBar()));
   mStatusTimer->start(500);

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
   if (settings->display.viewMode == DisplaySettings::Split) {
      mUi.actionViewSplit->setChecked(true);
   } else if (settings->display.viewMode == DisplaySettings::TV) {
      mUi.actionViewTV->setChecked(true);
   } else if (settings->display.viewMode == DisplaySettings::Gamepad1) {
      mUi.actionViewGamepad1->setChecked(true);
   } else if (settings->display.viewMode == DisplaySettings::Gamepad2) {
      mUi.actionViewGamepad2->setChecked(true);
   }
}

void
MainWindow::titleLoaded(quint64 id, const QString &name)
{
   setWindowTitle(QString("decaf-qt - %1").arg(name));
}

bool
MainWindow::loadFile(QString path)
{
   // You only get one chance to run a game out here buddy.
   mUi.actionOpen->setDisabled(true);
   for (auto i = 0u; i < mRecentFileActions.size(); ++i) {
      mRecentFileActions[i]->setDisabled(true);
   }

   // Tell decaf to start the game!
   mDecafInterface->startGame(path);

   // Update recent files list
   {
      QSettings settings;
      QStringList files = settings.value("recentFileList").toStringList();
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
   auto fileName = QFileDialog::getOpenFileName(this,
                                                tr("Open Application"), "",
                                                tr("RPX Files (*.rpx);;cos.xml (cos.xml);;"));
   if (!fileName.isEmpty()) {
      loadFile(fileName);
   }
}

void
MainWindow::openRecentFile()
{
   QAction *action = qobject_cast<QAction *>(sender());
   if (action) {
      loadFile(action->data().toString());
   }
}

void
MainWindow::updateRecentFileActions()
{
   QSettings settings;
   QStringList files = settings.value("recentFileList").toStringList();
   int numRecentFiles = qMin(files.size(), MaxRecentFiles);

   for (int i = 0; i < numRecentFiles; ++i) {
      auto text = QString("&%1 %2").arg(i + 1).arg(QFileInfo(files[i]).fileName());
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
   settings.display.viewMode = DisplaySettings::Split;
   mSettingsStorage->set(settings);
}

void
MainWindow::setViewModeTV()
{
   auto settings = *mSettingsStorage->get();
   settings.display.viewMode = DisplaySettings::TV;
   mSettingsStorage->set(settings);
}

void
MainWindow::setViewModeGamepad1()
{
   auto settings = *mSettingsStorage->get();
   settings.display.viewMode = DisplaySettings::Gamepad1;
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
MainWindow::openAboutDialog()
{
   AboutDialog dialog(this);
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
   mDecafInterface->shutdown();
   event->accept();
}

void
MainWindow::updateStatusBar()
{
   if (auto gpuDriver = decaf::getGraphicsDriver()) {
      mStatusFrameRate->setText(
         QString("FPS: %1")
         .arg(gpuDriver->getAverageFPS(), 2, 'f', 0));

      mStatusFrameTime->setText(
         QString("Frametime: %1ms")
         .arg(gpuDriver->getAverageFrametimeMS(), 7, 'f', 3));
   } else {
      mStatusFrameRate->setText("");
      mStatusFrameTime->setText("");
   }
}
