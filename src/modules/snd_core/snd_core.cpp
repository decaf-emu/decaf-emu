#include "snd_core.h"

Snd_Core::Snd_Core()
{
}

void
Snd_Core::initialise()
{
}

void
Snd_Core::RegisterFunctions()
{
   registerCoreFunctions();
   registerDeviceFunctions();
}
