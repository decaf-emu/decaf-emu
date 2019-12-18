#pragma once
#include <QAction>

struct DebuggerShortcuts
{
   QAction *toggleBreakpoint;
   QAction *navigateForward;
   QAction *navigateBackward;
   QAction *navigateToAddress;
   QAction *navigateToOperand;
};
