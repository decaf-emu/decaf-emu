#include "tcl.h"
#include "tcl_register.h"

namespace cafe::tcl
{

TCLStatus
TCLReadRegister(TCLRegisterID id,
                virt_ptr<uint32_t> outValue)
{
   if (id >= TCLRegisterID::Max) {
      return TCLStatus::InvalidArg;
   }

   return TCLStatus::NotInitialised;
}

TCLStatus
TCLWriteRegister(TCLRegisterID id,
                 uint32_t value)
{
   if (id >= TCLRegisterID::Max) {
      return TCLStatus::InvalidArg;
   }

   return TCLStatus::NotInitialised;
}

void
Library::registerRegisterSymbols()
{
   RegisterFunctionExport(TCLReadRegister);
   RegisterFunctionExport(TCLWriteRegister);
}

} // namespace cafe::tcl
