#include "vpad.h"
#include "vpad_gyro.h"

namespace cafe::vpad
{

float
VPADIsEnableGyroAccRevise(VPADChan chan)
{
   return 0.0f;
}

float
VPADIsEnableGyroZeroPlay(VPADChan chan)
{
   return 0.0f;
}

float
VPADIsEnableGyroZeroDrift(VPADChan chan)
{
   return 0.0f;
}

float
VPADIsEnableGyroDirRevise(VPADChan chan)
{
   return 0.0f;
}

void
Library::registerGyroSymbols()
{
   RegisterFunctionExport(VPADIsEnableGyroAccRevise);
   RegisterFunctionExport(VPADIsEnableGyroZeroPlay);
   RegisterFunctionExport(VPADIsEnableGyroZeroDrift);
   RegisterFunctionExport(VPADIsEnableGyroDirRevise);
}

} // namespace cafe::vpad
