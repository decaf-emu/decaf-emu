#pragma once
#include "decaf.h"
#include "replay.h"
#include "replayrenderwidget.h"

#include "ui_mainwindow.h"
#include <QErrorMessage>

class MainWindow : public QMainWindow
{
   Q_OBJECT

public:
   explicit MainWindow(QWidget *parent = 0);

   void startReplay();

protected slots:
   void onFileOpen();
   void onDecafStarted();

private:
   Ui_MainWindow ui;
   std::shared_ptr<ReplayFile> mReplay;
   QErrorMessage mErrorMessage;
   Decaf mDecaf;
};
