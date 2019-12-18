#pragma once
#include <QWidget>

#include "debugdata.h"

namespace Ui
{
class StackWindow;
}

class QAbstractItemModel;
struct DebuggerShortcuts;

class StackWindow : public QWidget
{
   Q_OBJECT

public:
   StackWindow(DebuggerShortcuts *debuggerShortcuts, QWidget *parent = nullptr);
   ~StackWindow();

   void setDebugData(DebugData *debugData);

signals:
   void navigateToTextAddress(uint32_t address);

public slots:
   void navigateToAddress(uint32_t address);
   void navigateForward();
   void navigateBackward();
   void navigateOperand();

protected slots:
   void updateStatus();

private:
   Ui::StackWindow *ui;

   DebugData *mDebugData = nullptr;
};
