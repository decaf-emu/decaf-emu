#include "tcl.h"
#include "tcl_driver.h"
#include "cafe/libraries/coreinit/coreinit_driver.h"
#include <libcpu/be2_struct.h>

namespace cafe::tcl
{

struct StaticDriverData
{
   be2_val<BOOL> isInitialised;
};

static virt_ptr<StaticDriverData>
sDriverData = nullptr;

namespace internal
{

void
initialiseTclDriver()
{
   // TODO: OSDriver_Register
   sDriverData->isInitialised = true;
}

bool
tclDriverInitialised()
{
   return sDriverData->isInitialised ? true : false;
}

} // namespace internal

void
Library::registerDriverSymbols()
{
   RegisterDataInternal(sDriverData);
}

} // namespace cafe::tcl
