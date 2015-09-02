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

SaveStatus
SAVEGetSharedDataTitlePath(uint64_t titleID, const char *dir, char *buffer, uint32_t bufferSize)
{
   auto titleLo = static_cast<uint32_t>(titleID & 0xffffffff);
   auto titleHi = static_cast<uint32_t>(titleID >> 32);
   auto result = snprintf(buffer, bufferSize, "/vol/storage_mlc01/sys/title/%08x/%08x/content/%s", titleHi, titleLo, dir);

   if (result < 0 || result >= static_cast<int>(bufferSize)) {
      return SaveStatus::FatalError;
   }

   return SaveStatus::OK;
}

void
NNSave::registerCoreFunctions()
{
   RegisterKernelFunction(SAVEInit);
   RegisterKernelFunction(SAVEShutdown);
   RegisterKernelFunction(SAVEInitSaveDir);
   RegisterKernelFunction(SAVEGetSharedDataTitlePath);
}
