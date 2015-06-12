#include "nn_save.h"
#include "nn_save_core.h"

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
NNSave::registerCoreFunctions()
{
   RegisterSystemFunction(SAVEInit);
   RegisterSystemFunction(SAVEShutdown);
}
