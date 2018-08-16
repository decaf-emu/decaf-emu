#pragma optimize("", off)
#include "cafe_loader_globals.h"

namespace cafe::loader
{

// 0xEFE00000 - 0xEFE01000 = Root RPX Name
static virt_ptr<char>
sLoadRpxName = virt_cast<char *>(virt_addr { 0xEFE00000 });

// 0xEFE01000 - 0xEFE02000 = Loader global variables
static virt_ptr<GlobalStorage>
sGlobalStorage = virt_cast<GlobalStorage *>(virt_addr { 0xEFE01000 });

// 0xEFE02000 - 0xEFE0A400 = Loader context & stack
static virt_ptr<ContextStorage>
sContextStorage = virt_cast<ContextStorage *>(virt_addr { 0xEFE02000 });

// 0xEFE0A400 - 0xEFE0A4C0 = Kernel <-> Loader data
static virt_ptr<KernelIpcStorage>
sKernelIpcStorage = virt_cast<KernelIpcStorage *>(virt_addr { 0xEFE0A400 });

void
setLoadRpxName(std::string_view name)
{
   std::memcpy(sLoadRpxName.getRawPointer(),
               name.data(),
               name.size());
   sLoadRpxName[name.size()] = char { 0 };
}

virt_ptr<char>
getLoadRpxName()
{
   return sLoadRpxName;
}

virt_ptr<GlobalStorage>
getGlobalStorage()
{
   return sGlobalStorage;
}

virt_ptr<ContextStorage>
getContextStorage()
{
   return sContextStorage;
}

virt_ptr<KernelIpcStorage>
getKernelIpcStorage()
{
   return sKernelIpcStorage;
}

} // namespace cafe::loader
