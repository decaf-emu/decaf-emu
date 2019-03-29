#pragma once
#include <QWidget>
#include <memory>

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

   void setDebugData(DebugData *debugData);

protected slots:
   void threadsViewDoubleClicked(const QModelIndex &index);

private:
   std::unique_ptr<Ui::ThreadsWindow> ui;

   DebugData *mDebugData = nullptr;
   ThreadsModel *mThreadsModel = nullptr;
};
