#include "cpu.h"
#include <vector>

namespace cpu
{

static std::vector<KernelCallEntry>
sKernelCalls;

uint32_t
registerKernelCall(const KernelCallEntry &entry)
{
   sKernelCalls.push_back(entry);
   return static_cast<uint32_t>(sKernelCalls.size() - 1);
}

KernelCallEntry *
getKernelCall(uint32_t id)
{
   if (id >= sKernelCalls.size()) {
      return nullptr;
   }

   return &sKernelCalls[id];
}

} // namespace cpu
