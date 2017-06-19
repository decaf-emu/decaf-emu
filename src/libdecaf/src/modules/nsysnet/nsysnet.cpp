#include "nsysnet.h"

namespace nsysnet
{

Module::Module()
{
}

void
Module::initialise()
{
   initialiseSocketLib();
}

void
Module::RegisterFunctions()
{
   registerEndianFunctions();
   registerSocketLibFunctions();
   registerSSLFunctions();
}

} // namespace nsysnet
