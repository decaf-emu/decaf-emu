#include "nn_acp.h"

namespace nn
{

namespace acp
{

void
Module::initialise()
{
}

void
Module::RegisterFunctions()
{
   registerDeviceFunctions();
   registerTitleFunctions();
}

} // namespace acp

} // namespace nn
