#pragma once
#include "debugui_window.h"

#include <chrono>
#include <vector>

namespace cpu
{
namespace jit
{
struct CodeBlock;
}
}

namespace debugui
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
   bool mNeedProfileListUpdate = true;
   std::vector<cpu::jit::CodeBlock *> mProfileList;
};

} // namespace debugui
