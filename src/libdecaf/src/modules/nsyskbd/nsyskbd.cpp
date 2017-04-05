#include "nsyskbd.h"

namespace nsyskbd
{

void
Module::initialise()
{
}

void
Module::RegisterFunctions()
{
   registerKprFunctions();
   registerSkbdFunctions();
}

} // namespace nsyskbd
