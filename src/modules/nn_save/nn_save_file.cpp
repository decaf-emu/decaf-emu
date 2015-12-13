#include "nn_save.h"
#include "nn_save_file.h"

FSStatus
SAVEOpenFile(FSClient *client,
             FSCmdBlock *block,
             uint8_t accountSlotNo,
             const char *path,
             const char *mode,
             be_val<FSFileHandle> *handle,
             uint32_t flags)
{
   // TODO: SAVEOpenFile
   return FSStatus::NotFound;
}

FSStatus
SAVEOpenFileAsync(FSClient *client,
                  FSCmdBlock *block,
                  uint8_t accountSlotNo,
                  const char *path,
                  const char *mode,
                  be_val<FSFileHandle> *handle,
                  uint32_t flags,
                  FSAsyncData *asyncData)
{
   // TODO: SAVEOpenFileAsync
   return FSStatus::FatalError;
}

void
NN_save::registerFileFunctions()
{
   RegisterKernelFunction(SAVEOpenFile);
   RegisterKernelFunction(SAVEOpenFileAsync);
}
