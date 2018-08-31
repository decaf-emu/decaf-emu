#pragma once
#include "debugger_ui_window.h"
#include <libcpu/state.h>

namespace debugger
{

namespace ui
{

class RegistersWindow : public Window
{
public:
   RegistersWindow(const std::string &name);
   virtual ~RegistersWindow() = default;

   void update();
   virtual void draw();

private:
   cpu::CoreRegs mCurrentRegisters;
   cpu::CoreRegs mPreviousRegisters;
   virt_ptr<cafe::coreinit::OSThread> mLastActiveThread;
   unsigned mLastResumeCount;
};

} // namespace ui

} // namespace debugger
