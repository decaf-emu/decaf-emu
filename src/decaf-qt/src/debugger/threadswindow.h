#pragma once
#include <QWidget>

namespace Ui
{
class ThreadsWindow;
}

class DebugData;
class ThreadsModel;

class ThreadsWindow : public QWidget
{
   Q_OBJECT

public:
   explicit ThreadsWindow(QWidget *parent = nullptr);
   ~ThreadsWindow();

   void setDebugData(DebugData *debugData);

protected slots:
   void threadsViewDoubleClicked(const QModelIndex &index);

private:
   Ui::ThreadsWindow *ui;

   DebugData *mDebugData = nullptr;
   ThreadsModel *mThreadsModel = nullptr;
};
