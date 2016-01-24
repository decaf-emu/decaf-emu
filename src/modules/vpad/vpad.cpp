#include "vpad.h"

namespace vpad
{

void
Module::initialise()
{
}

void
Module::RegisterFunctions()
{
   registerCoreFunctions();
   registerStatusFunctions();
}

} // namespace vpad
