#pragma once
#include <QWidget>

namespace Ui
{
class DisassemblyWindow;
}

class DebugData;
struct DebuggerShortcuts;

class DisassemblyWindow : public QWidget
{
   Q_OBJECT

public:
   explicit DisassemblyWindow(DebuggerShortcuts *debuggerShortcuts, QWidget *parent = nullptr);
   ~DisassemblyWindow();

   void setDebugData(DebugData *debugData);

public slots:
   void navigateToAddress(uint32_t address);
   void navigateForward();
   void navigateBackward();
   void navigateOperand();
   void toggleBreakpointUnderCursor();

private:
   Ui::DisassemblyWindow *ui;

   DebugData *mDebugData = nullptr;
};
