#pragma once
#include "modules/coreinit/coreinit_fs.h"

FSStatus
SAVEOpenFile(FSClient *client,
             FSCmdBlock *block,
             uint8_t accountSlotNo,
             const char *path,
             const char *mode,
             be_val<FSFileHandle> *handle,
             uint32_t flags);

FSStatus
SAVEOpenFileAsync(FSClient *client,
                  FSCmdBlock *block,
                  uint8_t accountSlotNo,
                  const char *path,
                  const char *mode,
                  be_val<FSFileHandle> *handle,
                  uint32_t flags,
                  FSAsyncData *asyncData);
