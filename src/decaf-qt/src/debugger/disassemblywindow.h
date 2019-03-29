#pragma once
#include <QWidget>
#include <memory>

namespace Ui
{
class DisassemblyWindow;
}

class DebugData;

class DisassemblyWindow : public QWidget
{
   Q_OBJECT

public:
   explicit DisassemblyWindow(QWidget *parent = nullptr);

   void setDebugData(DebugData *debugData);

public slots:
   void navigateToAddress(uint32_t address);
   void toggleBreakpointUnderCursor();

private:
   std::unique_ptr<Ui::DisassemblyWindow> ui;

   DebugData *mDebugData = nullptr;
};
