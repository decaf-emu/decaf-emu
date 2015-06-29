#include "nn_save.h"
#include "nn_save_core.h"

SaveStatus
SAVEInit()
{
   return SaveStatus::OK;
}

void
SAVEShutdown()
{
}

SaveStatus
SAVEInitSaveDir(uint8_t userID)
{
   return SaveStatus::OK;
}

void
NNSave::registerCoreFunctions()
{
   RegisterSystemFunction(SAVEInit);
   RegisterSystemFunction(SAVEShutdown);
   RegisterSystemFunction(SAVEInitSaveDir);
}
