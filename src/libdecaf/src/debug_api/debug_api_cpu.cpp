#include "decaf_debug_api.h"

#include <libcpu/cpu_breakpoints.h>

namespace decaf::debug
{

void
sampleCpuBreakpoints(std::vector<CpuBreakpoint> &breakpoints)
{
   breakpoints.clear();

   if (auto list = cpu::getBreakpoints()) {
      for (auto &bp : *list) {
         breakpoints.push_back({
            bp.type == bp.SingleFire ? CpuBreakpoint::SingleFire : CpuBreakpoint::MultiFire,
            VirtualAddress { bp.address },
            bp.savedCode,
         });
      }
   }
}

} // namespace decaf::debug
