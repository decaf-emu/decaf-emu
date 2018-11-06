#include "ios_nn.h"
#include "ios_nn_criticalsection.h"

namespace nn
{

void
initialiseProcess()
{
   internal::initialiseProcessCriticalSectionData();
}

void
uninitialiseProcess()
{
   internal::freeProcessCriticalSectionData();
}

} // namespace nn
