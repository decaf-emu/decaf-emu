#pragma once
#include "debugger_ui_window.h"

#include <chrono>
#include <vector>

namespace cpu
{
namespace jit
{
struct CodeBlock;
}
}

namespace debugger
{

namespace ui
{

class StatsWindow : public Window
{
public:
   StatsWindow(const std::string &name);
   virtual ~StatsWindow() = default;

   virtual void
   draw() override;

   void
   update();

private:
   std::chrono::time_point<std::chrono::system_clock> mLastProfileListUpdate;
   bool mNeedProfileListUpdate;
   std::vector<cpu::jit::CodeBlock *> mProfileList;
};

} // namespace ui

} // namespace debugger
