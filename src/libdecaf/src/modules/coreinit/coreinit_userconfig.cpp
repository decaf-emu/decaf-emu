#include "coreinit.h"
#include "coreinit_ios.h"
#include "coreinit_ipcbufpool.h"
#include "coreinit_mutex.h"
#include "coreinit_userconfig.h"

namespace coreinit
{

using kernel::ios::usr_cfg::UCReadSysConfigRequest;
using kernel::ios::usr_cfg::UCWriteSysConfigRequest;

struct UserConfigData
{
   OSMutex lock;

   static constexpr uint32_t SmallMessageCount = 0x100;
   static constexpr uint32_t SmallMessageSize = 0x80;

   static constexpr uint32_t LargeMessageCount = 0x40;
   static constexpr uint32_t LargeMessageSize = 0x1000;

   bool initialised;

   IPCBufPool *smallMessagePool;
   IPCBufPool *largeMessagePool;

   std::array<uint8_t, SmallMessageCount * SmallMessageSize> smallMessageBuffer;
   std::array<uint8_t, LargeMessageCount * LargeMessageSize> largeMessageBuffer;

   be_val<uint32_t> smallMessageCount;
   be_val<uint32_t> largeMessageCount;
};

static UserConfigData *
sUserConfigData;

static IOSAsyncCallbackFn
sIosAsyncCallbackFn = nullptr;

namespace internal
{

static void *
ucAllocateMessage(uint32_t size)
{
   void *message = nullptr;

   if (size == 0) {
      return nullptr;
   } else if (size <= UserConfigData::SmallMessageSize) {
      message = IPCBufPoolAllocate(sUserConfigData->smallMessagePool, size);
   } else {
      message = IPCBufPoolAllocate(sUserConfigData->largeMessagePool, size);
   }

   std::memset(message, 0, size);
   return message;
}


static void
ucFreeMessage(void *message)
{
   IPCBufPoolFree(sUserConfigData->smallMessagePool, message);
   IPCBufPoolFree(sUserConfigData->largeMessagePool, message);
}


static UCError
ucSetupAsyncParams(UCCommand command,
                   uint32_t unk_r4,
                   uint32_t count,
                   UCSysConfig *settings,
                   IOSVec *vecs,
                   UCAsyncParams *asyncParams)
{
   if (!settings || !vecs || !asyncParams) {
      return UCError::InvalidBuffer;
   }

   asyncParams->command = command;
   asyncParams->unk0x0C = unk_r4;
   asyncParams->count = count;
   asyncParams->settings = settings;
   asyncParams->vecs = vecs;
   return UCError::OK;
}


static UCError
ucHandleIosResult(UCError result,
                  UCCommand command,
                  uint32_t unk_r5,
                  uint32_t count,
                  UCSysConfig *settings,
                  IOSVec *vecs,
                  UCAsyncParams *asyncParams,
                  UCAsyncCallback callback,
                  void *callbackContext)
{
   if (!settings && !vecs) {
      if (result == IOSError::OK) {
         return UCError::InvalidBuffer;
      } else {
         return static_cast<UCError>(result);
      }
   }

   if (result != UCError::OutOfMemory) {
      if (settings && vecs) {
         if (result == UCError::OK && asyncParams) {
            // Return as we have a pending async result...!
            return UCError::OK;
         }

         if (command == UCCommand::ReadSysConfig) {
            auto request = reinterpret_cast<UCReadSysConfigRequest *>(vecs[0].paddr.get());

            for (auto i = 0u; i < count; ++i) {
               settings[i].error = request->settings[i].error;

               if (settings[i].error) {
                  result = settings[i].error;
               }

               if (!settings[i].data) {
                  result = UCError::InvalidBuffer;
               }

               if (settings[i].error == UCError::OK) {
                  switch (settings[i].dataSize) {
                  case 1:
                     *be_ptr<uint8_t> { settings[i].data } = *be_ptr<uint8_t> { vecs[i + 1].paddr };
                     break;
                  case 2:
                     *be_ptr<uint16_t> { settings[i].data } = *be_ptr<uint16_t> { vecs[i + 1].paddr };
                     break;
                  case 4:
                     *be_ptr<uint32_t> { settings[i].data } = *be_ptr<uint32_t> { vecs[i + 1].paddr };
                     break;
                  default:
                     std::memset(settings[i].data, 0, 4); // why???
                     std::memcpy(settings[i].data, vecs[i + 1].paddr, settings[i].dataSize);
                  }
               }
            }
         } else if (command == UCCommand::WriteSysConfig) {
            auto request = reinterpret_cast<UCWriteSysConfigRequest *>(vecs[0].paddr.get());

            for (auto i = 0u; i < count; ++i) {
               settings[i].error = request->settings[i].error;

               if (settings[i].error) {
                  result = settings[i].error;
               }
            }
         } else {
            decaf_abort(fmt::format("Unimplemented result handler for UCCommand {}", command));
         }
      }

      if (callback) {
         callback(result, command, count, settings, callbackContext);
      }
   }

   if (vecs) {
      for (auto i = 0u; i < count + 1; ++i) {
         internal::ucFreeMessage(vecs[i].paddr);
      }

      internal::ucFreeMessage(vecs);
   }

   return static_cast<UCError>(result);
}


static void
ucIosAsyncCallback(IOSError status, void *context)
{
   auto asyncParams = reinterpret_cast<UCAsyncParams *>(context);
   ucHandleIosResult(UCError::OK,
                     asyncParams->command,
                     asyncParams->unk0x0C,
                     asyncParams->count,
                     asyncParams->settings,
                     asyncParams->vecs,
                     nullptr,
                     asyncParams->callback,
                     asyncParams->context);
}

} // namespace internal


UCError
UCOpen()
{
   OSLockMutex(&sUserConfigData->lock);

   if (!sUserConfigData->initialised) {
      if (!sUserConfigData->smallMessagePool) {
         sUserConfigData->smallMessagePool = IPCBufPoolCreate(sUserConfigData->smallMessageBuffer.data(),
                                                              static_cast<uint32_t>(sUserConfigData->smallMessageBuffer.size()),
                                                              UserConfigData::SmallMessageSize,
                                                              &sUserConfigData->smallMessageCount,
                                                              1);
      }

      if (!sUserConfigData->largeMessagePool) {
         sUserConfigData->largeMessagePool = IPCBufPoolCreate(sUserConfigData->largeMessageBuffer.data(),
                                                              static_cast<uint32_t>(sUserConfigData->largeMessageBuffer.size()),
                                                              UserConfigData::LargeMessageSize,
                                                              &sUserConfigData->largeMessageCount,
                                                              1);
      }

      if (sUserConfigData->smallMessagePool && sUserConfigData->largeMessagePool) {
         sUserConfigData->initialised = true;
      }
   }

   OSUnlockMutex(&sUserConfigData->lock);

   if (!sUserConfigData->initialised) {
      return UCError::Error;
   }

   return static_cast<UCError>(IOS_Open("/dev/usr_cfg", IOSOpenMode::None));
}


UCError
UCClose(IOSHandle handle)
{
   return static_cast<UCError>(IOS_Close(handle));
}


UCError
UCDeleteSysConfig(IOSHandle handle,
                  uint32_t count,
                  UCSysConfig *settings)
{
   return UCError::OK;
}


UCError
UCReadSysConfig(IOSHandle handle,
                uint32_t count,
                UCSysConfig *settings)
{
   return UCReadSysConfigAsync(handle, count, settings, nullptr);
}


UCError
UCReadSysConfigAsync(IOSHandle handle,
                     uint32_t count,
                     UCSysConfig *settings,
                     UCAsyncParams *asyncParams)
{
   auto result = UCError::OK;

   if (!settings) {
      result = UCError::InvalidBuffer;
      goto fail;
   }

   auto msgBufSize = (sizeof(UCSysConfig) * (count + 1)) + sizeof(UCReadSysConfigRequest);
   auto msgBuf = internal::ucAllocateMessage(msgBufSize);
   if (!msgBuf) {
      result = UCError::OutOfMemory;
      goto fail;
   }

   auto request = reinterpret_cast<UCReadSysConfigRequest *>(msgBuf);
   request->count = count;
   request->size = sizeof(UCSysConfig);
   std::memcpy(request->settings, settings, sizeof(UCSysConfig) * count);

   auto vecBufSize = sizeof(IOSVec) * (count + 1);
   auto vecBuf = internal::ucAllocateMessage(vecBufSize);
   if (!vecBuf) {
      result = UCError::OutOfMemory;
      goto fail;
   }

   auto vecs = reinterpret_cast<IOSVec *>(vecBuf);
   vecs[0].paddr = msgBuf;
   vecs[0].len = msgBufSize;

   for (auto i = 0u; i < count; ++i) {
      auto size = settings[i].dataSize;
      vecs[1 + i].len = size;

      if (size > 0) {
         vecs[1 + i].paddr = internal::ucAllocateMessage(size);
         if (!vecs[1 + i].paddr) {
            result = UCError::OutOfMemory;
            goto fail;
         }
      } else {
         vecs[1 + i].paddr = nullptr;
      }
   }

   if (!asyncParams) {
      result = static_cast<UCError>(IOS_Ioctlv(handle,
                                               UCCommand::ReadSysConfig,
                                               0,
                                               count + 1,
                                               vecs));
   } else {
      internal::ucSetupAsyncParams(UCCommand::ReadSysConfig,
                                   0,
                                   count,
                                   settings,
                                   vecs,
                                   asyncParams);

      result = static_cast<UCError>(IOS_IoctlvAsync(handle,
                                                    UCCommand::ReadSysConfig,
                                                    0,
                                                    count + 1,
                                                    vecs,
                                                    sIosAsyncCallbackFn,
                                                    asyncParams));
   }

   goto out;

fail:
   if (msgBuf) {
      internal::ucFreeMessage(msgBuf);
      msgBuf = nullptr;
   }

   if (vecBuf) {
      for (auto i = 0u; i < count; ++i) {
         if (vecs[1 + i].paddr) {
            internal::ucFreeMessage(vecs[1 + i].paddr);
         }
      }

      internal::ucFreeMessage(vecBuf);
      vecBuf = nullptr;
      vecs = nullptr;
   }

out:
   return internal::ucHandleIosResult(result,
                                      UCCommand::ReadSysConfig,
                                      0,
                                      count,
                                      settings,
                                      vecs,
                                      asyncParams,
                                      nullptr,
                                      nullptr);
}


UCError
UCWriteSysConfig(IOSHandle handle,
                 uint32_t count,
                 UCSysConfig *settings)
{
   return UCWriteSysConfigAsync(handle, count, settings, nullptr);
}


UCError
UCWriteSysConfigAsync(IOSHandle handle,
                      uint32_t count,
                      UCSysConfig *settings,
                      UCAsyncParams *asyncParams)
{
   auto result = UCError::OK;

   if (!settings) {
      result = UCError::InvalidBuffer;
      goto fail;
   }

   auto msgBufSize = (sizeof(UCSysConfig) * (count + 1)) + sizeof(UCWriteSysConfigRequest);
   auto msgBuf = internal::ucAllocateMessage(msgBufSize);
   if (!msgBuf) {
      result = UCError::OutOfMemory;
      goto fail;
   }

   auto request = reinterpret_cast<UCWriteSysConfigRequest *>(msgBuf);
   request->count = count;
   request->size = sizeof(UCSysConfig);
   std::memcpy(request->settings, settings, sizeof(UCSysConfig) * count);

   auto vecBufSize = sizeof(IOSVec) * (count + 1);
   auto vecBuf = internal::ucAllocateMessage(vecBufSize);
   if (!vecBuf) {
      result = UCError::OutOfMemory;
      goto fail;
   }

   auto vecs = reinterpret_cast<IOSVec *>(vecBuf);
   vecs[0].paddr = msgBuf;
   vecs[0].len = msgBufSize;

   for (auto i = 0u; i < count; ++i) {
      auto size = settings[i].dataSize;
      vecs[1 + i].len = size;

      if (size > 0) {
         vecs[1 + i].paddr = internal::ucAllocateMessage(size);
         if (!vecs[1 + i].paddr) {
            result = UCError::OutOfMemory;
            goto fail;
         }
      } else {
         vecs[1 + i].paddr = nullptr;
      }
   }

   if (!asyncParams) {
      result = static_cast<UCError>(IOS_Ioctlv(handle,
                                               UCCommand::WriteSysConfig,
                                               0,
                                               count + 1,
                                               vecs));
   } else {
      internal::ucSetupAsyncParams(UCCommand::WriteSysConfig,
                                   0,
                                   count,
                                   settings,
                                   vecs,
                                   asyncParams);

      result = static_cast<UCError>(IOS_IoctlvAsync(handle,
                                                    UCCommand::WriteSysConfig,
                                                    0,
                                                    count + 1,
                                                    vecs,
                                                    sIosAsyncCallbackFn,
                                                    asyncParams));
   }

   goto out;

fail:
   if (msgBuf) {
      internal::ucFreeMessage(msgBuf);
      msgBuf = nullptr;
   }

   if (vecBuf) {
      for (auto i = 0u; i < count; ++i) {
         if (vecs[1 + i].paddr) {
            internal::ucFreeMessage(vecs[1 + i].paddr);
         }
      }

      internal::ucFreeMessage(vecBuf);
      vecBuf = nullptr;
      vecs = nullptr;
   }

out:
   return internal::ucHandleIosResult(result,
                                      UCCommand::WriteSysConfig,
                                      0,
                                      count,
                                      settings,
                                      vecs,
                                      asyncParams,
                                      nullptr,
                                      nullptr);
}


void
Module::initialiseUserConfig()
{
   OSInitMutex(&sUserConfigData->lock);
}

void
Module::registerUserConfigFunctions()
{
   RegisterKernelFunction(UCOpen);
   RegisterKernelFunction(UCClose);
   RegisterKernelFunction(UCReadSysConfig);
   RegisterKernelFunction(UCReadSysConfigAsync);
   RegisterKernelFunction(UCWriteSysConfig);
   RegisterKernelFunction(UCWriteSysConfigAsync);

   RegisterInternalData(sUserConfigData);
   RegisterInternalFunction(internal::ucIosAsyncCallback, sIosAsyncCallbackFn);
}

} // namespace coreinit
