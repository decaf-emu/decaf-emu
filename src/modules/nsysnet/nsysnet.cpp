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
   registerSocketLibFunctions();
}

} // namespace nsysnet
