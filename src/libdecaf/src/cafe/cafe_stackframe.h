#pragma once
#include <libcpu/cpu.h>

namespace cafe
{

class StackFrame
{
public:
   StackFrame()
   {
      auto core = cpu::this_core::state();

      // Update stack pointer
      auto backchain = core->gpr[1];
      core->gpr[1] -= 8;

      // Write stack frame
      auto frame = virt_cast<uint32_t *>(virt_addr { core->gpr[1] });
      frame[0] = backchain;
      frame[1] = core->lr;

      // Setup lr to point to current function
      core->lr = core->nia;
   }

   ~StackFrame()
   {
      auto core = cpu::this_core::state();

      // Restore from frame
      auto frame = virt_cast<uint32_t *>(virt_addr { core->gpr[1] });
      core->gpr[1] = frame[0];
      core->lr = frame[1];
   }
};

} // namespace cafe
