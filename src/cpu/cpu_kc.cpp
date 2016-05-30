#include "cpu.h"
#include <vector>

namespace cpu
{

static std::vector<kernel_call_entry>
gKernelCalls;

uint32_t
register_kernel_call(const kernel_call_entry &entry)
{
   gKernelCalls.push_back(entry);
   return static_cast<uint32_t>(gKernelCalls.size() - 1);
}

kernel_call_entry *
get_kernel_call(uint32_t id)
{
   if (id >= gKernelCalls.size()) {
      return nullptr;
   }

   return &gKernelCalls[id];
}

} // namespace cpu
