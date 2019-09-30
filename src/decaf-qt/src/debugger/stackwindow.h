#pragma once
#include <QWidget>

#include "debugdata.h"

namespace Ui
{
class StackWindow;
}

class QAbstractItemModel;

class StackWindow : public QWidget
{
   Q_OBJECT

public:
   StackWindow(QWidget *parent = nullptr);
   ~StackWindow();

   void setDebugData(DebugData *debugData);

protected slots:
   void updateStatus();

private:
   Ui::StackWindow *ui;

   DebugData *mDebugData = nullptr;
};
