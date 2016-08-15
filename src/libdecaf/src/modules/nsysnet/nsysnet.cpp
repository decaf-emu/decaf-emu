#include "nsysnet.h"

namespace nsysnet
{

Module::Module()
{
}

void
Module::initialise()
{
}

void
Module::RegisterFunctions()
{
   registerEndianFunctions();
   registerSocketLibFunctions();
   registerSSLFunctions();
}

} // namespace nsysnet
