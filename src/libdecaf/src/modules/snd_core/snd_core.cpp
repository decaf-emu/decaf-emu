#include "snd_core.h"

namespace snd_core
{

void
Module::initialise()
{
}

void
Module::RegisterFunctions()
{
   registerAiFunctions();
   registerCoreFunctions();
   registerDeviceFunctions();
   registerVSFunctions();
}

} // namespace snd_core
