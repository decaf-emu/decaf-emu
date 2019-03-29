#pragma once
#include <QWidget>
#include <memory>

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

   void setDebugData(DebugData *debugData);

protected slots:
   void updateStatus();

private:
   std::unique_ptr<Ui::StackWindow> ui;

   DebugData *mDebugData = nullptr;
};
