#include "disassemblywindow.h"
#include "ui_disassemblywindow.h"

#include "debugdata.h"

DisassemblyWindow::DisassemblyWindow(QWidget *parent) :
   QWidget(parent),
   ui(new Ui::DisassemblyWindow { })
{
   ui->setupUi(this);
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
DisassemblyWindow::toggleBreakpointUnderCursor()
{
   ui->disassemblyWidget->toggleBreakpointUnderCursor();
}
