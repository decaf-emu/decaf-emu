#include "padscore.h"

namespace padscore
{

void
Module::initialise()
{
}

void
Module::RegisterFunctions()
{
   registerKPADFunctions();
   registerKPADStatusFunctions();
   registerWPADFunctions();
}

} // namespace padscore
