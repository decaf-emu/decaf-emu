#include "vpad.h"
#include "vpad_motor.h"

namespace cafe::vpad
{

int32_t
VPADControlMotor(VPADChan chan,
                 virt_ptr<void> buffer,
                 uint32_t size)
{
   return 0;
}

void
VPADStopMotor(VPADChan chan)
{
}

void
Library::registerMotorSymbols()
{
   RegisterFunctionExport(VPADControlMotor);
   RegisterFunctionExport(VPADStopMotor);
}

} // namespace cafe::vpad
