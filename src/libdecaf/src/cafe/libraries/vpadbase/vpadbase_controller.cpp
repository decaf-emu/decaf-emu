#include "vpadbase.h"
#include "vpadbase_controller.h"

#include "cafe/libraries/cafe_hle_stub.h"

namespace cafe::vpadbase
{

BOOL
VPADBASEGetHeadphoneStatus(int32_t chan)
{
   decaf_warn_stub();
   return FALSE;
}

void
Library::registerControllerSymbols()
{
   RegisterFunctionExport(VPADBASEGetHeadphoneStatus);
}

} // namespace cafe::vpadbase
