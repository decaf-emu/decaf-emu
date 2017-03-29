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
   registerMixFunctions();
   registerVoiceFunctions();
   registerVSFunctions();
   registerFXFunctions();
}

} // namespace snd_core
