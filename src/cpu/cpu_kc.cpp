#include "cpu.h"
#include <vector>

namespace cpu
{

static std::vector<KernelCallEntry>
gKernelCalls;

uint32_t
registerKernelCall(const KernelCallEntry &entry)
{
   gKernelCalls.push_back(entry);
   return static_cast<uint32_t>(gKernelCalls.size() - 1);
}

KernelCallEntry *
getKernelCall(uint32_t id)
{
   if (id >= gKernelCalls.size()) {
      return nullptr;
   }

   return &gKernelCalls[id];
}

} // namespace cpu
