#include "padscore.h"

PadScore::PadScore()
{
}

void
PadScore::initialise()
{
}

void
PadScore::RegisterFunctions()
{
   registerKPADFunctions();
   registerKPADStatusFunctions();
   registerWPADFunctions();
}
