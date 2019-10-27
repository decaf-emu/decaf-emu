#include "cafe_ghs_typeinfo.h"
#include "cafe_ghs_malloc.h"

#include "cafe/libraries/coreinit/coreinit_ghs.h"
#include "cafe/libraries/coreinit/coreinit_osreport.h"

namespace cafe::ghs
{

void
std_typeinfo_Destructor(virt_ptr<void> self,
                        ghs::DestructorFlags flags)
{
   if (self && (flags & ghs::DestructorFlags::FreeMemory)) {
      ghs::free(self);
   }
}

void
pure_virtual_called()
{
   coreinit::internal::OSPanic("ghs", 0, "__pure_virtual_called");
   coreinit::ghs_exit(6);
}

} // namespace cafe::ghs
