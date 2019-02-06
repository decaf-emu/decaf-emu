#include "coreinit.h"
#include "coreinit_codegen.h"

#include "cafe/kernel/cafe_kernel_mmu.h"

namespace cafe::coreinit
{

void
OSGetCodegenVirtAddrRange(virt_ptr<virt_addr> outAddress,
                          virt_ptr<uint32_t> outSize)
{
   auto range = kernel::getCodeGenVirtualRange();

   if (outAddress) {
      *outAddress = range.first;
   }

   if (outSize) {
      *outSize = range.second;
   }
}

void
Library::registerCodeGenSymbols()
{
   RegisterFunctionExport(OSGetCodegenVirtAddrRange);
}

} // namespace cafe::coreinit
