#include "debugdata.h"
#include "debuggershortcuts.h"
#include "disassemblywindow.h"
#include "ui_disassemblywindow.h"

DisassemblyWindow::DisassemblyWindow(DebuggerShortcuts *debuggerShortcuts,
                                     QWidget *parent) :
   QWidget(parent),
   ui(new Ui::DisassemblyWindow { })
{
   ui->setupUi(this);

   ui->disassemblyWidget->addAction(debuggerShortcuts->toggleBreakpoint);
   ui->disassemblyWidget->addAction(debuggerShortcuts->navigateBackward);
   ui->disassemblyWidget->addAction(debuggerShortcuts->navigateForward);
   ui->disassemblyWidget->addAction(debuggerShortcuts->navigateToAddress);
   ui->disassemblyWidget->addAction(debuggerShortcuts->navigateToOperand);
}

DisassemblyWindow::~DisassemblyWindow()
{
   delete ui;
}

void
DisassemblyWindow::setDebugData(DebugData *debugData)
{
   mDebugData = debugData;
   ui->disassemblyWidget->setDebugData(debugData);
   ui->disassemblyWidget->setAddressRange(0, 0xFFFFFFFF);
   ui->disassemblyWidget->navigateToAddress(0x02000000);
}

void
DisassemblyWindow::navigateToAddress(uint32_t address)
{
   ui->disassemblyWidget->navigateToAddress(address);
}

void
DisassemblyWindow::navigateForward()
{
   ui->disassemblyWidget->navigateForward();
}

void
DisassemblyWindow::navigateBackward()
{
   ui->disassemblyWidget->navigateBackward();
}

void
DisassemblyWindow::navigateOperand()
{
   ui->disassemblyWidget->followSymbolUnderCursor();
}

void
DisassemblyWindow::toggleBreakpointUnderCursor()
{
   ui->disassemblyWidget->toggleBreakpointUnderCursor();
}
