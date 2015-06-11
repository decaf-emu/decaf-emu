#include "coreinit.h"
#include "coreinit_save.h"

SaveError
SAVEInit()
{
   return SaveError::OK;
}

void
SAVEShutdown()
{
}

void
CoreInit::registerSaveFunctions()
{
   RegisterSystemFunction(SAVEInit);
   RegisterSystemFunction(SAVEShutdown);
}
