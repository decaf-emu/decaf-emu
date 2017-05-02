#include "replayrunner.h"
#include "mainwindow.h"
#include "replaycommandsmodel.h"

#include <QFileDialog>
#include <QOpenGLContext>
#include <QOffscreenSurface>

MainWindow::MainWindow(QWidget *parent) :
   QMainWindow(parent),
   mErrorMessage(this)
{
   ui.setupUi(this);

   connect(&mDecaf, &Decaf::started, this, &MainWindow::onDecafStarted);
   mDecaf.start();
}

void MainWindow::onFileOpen()
{
   auto fileName = QFileDialog::getOpenFileName(this,
                                                tr("Open PM4 Replay"), "",
                                                tr("PM4 Replay (*.pm4);;All Files (*)"));

   if (fileName.isEmpty()) {
      return;
   }

   mReplay = openReplay(fileName.toStdString());

   if (!mReplay) {
      mErrorMessage.showMessage(QString { "%1 is not a valid pm4 replay file." }.arg(fileName));
      return;
   }

   // TODO: Thread buildReplayIndex...
   buildReplayIndex(mReplay);

   auto model = new ReplayCommandModel { mReplay, nullptr };
   ui.tableView->setModel(model);

   auto runner = new ReplayRunner { &mDecaf, mReplay };
   connect(runner, &ReplayRunner::frameFinished, ui.openGLWidget, &ReplayRenderWidget::displayFrame);
   runner->moveToThread(mDecaf.thread());
   QMetaObject::invokeMethod(runner, "initialise");
   QMetaObject::invokeMethod(runner, "runFrame");

   QTimer *timer = new QTimer(this);
   connect(timer, &QTimer::timeout, runner, &ReplayRunner::runFrame);
   timer->start(100);
}

void MainWindow::startReplay()
{
}

void MainWindow::onDecafStarted()
{
   ui.openGLWidget->doneCurrent();

   auto surface = new QOffscreenSurface { };
   surface->setFormat(ui.openGLWidget->context()->format());
   surface->create();

   auto context = new QOpenGLContext { };
   context->setFormat(surface->format());
   context->setShareContext(ui.openGLWidget->context());
   context->create();

   surface->moveToThread(mDecaf.thread());
   context->moveToThread(mDecaf.thread());
   mDecaf.setContext(surface, context);

   ui.openGLWidget->makeCurrent();
}
