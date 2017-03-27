#include "coreinit.h"
#include "coreinit_ios.h"
#include "coreinit_ipc.h"
#include "coreinit_thread.h"

namespace coreinit
{

namespace internal
{

static IOSError
ipcPrepareOpenRequest(IPCDriver *ipcDriver,
                      IPCDriverRequest *ipcRequest,
                      const char *device,
                      int mode);

static IOSError
ipcPrepareIoctlRequest(IPCDriver *ipcDriver,
                       IPCDriverRequest *ipcRequest,
                       uint32_t ioctlRequest,
                       void *inBuf,
                       uint32_t inLen,
                       void *outBuf,
                       uint32_t outLen);

static IOSError
ipcPrepareIoctlvRequest(IPCDriver *ipcDriver,
                        IPCDriverRequest *ipcRequest,
                        uint32_t ioctlvRequest,
                        uint32_t vecIn,
                        uint32_t vecOut,
                        IOSVec *vec);

} // namespace internal


/**
 * Sends an IOS Open command over IPC and waits for the response.
 *
 * \return
 * Returns an IOSHandle (when the result is > 0) or an IOSError code otherwise.
 */
IOSError
IOS_Open(const char *device,
         IOSOpenMode mode)
{
   IPCDriverRequest *ipcRequest = nullptr;
   auto ipcDriver = internal::getIPCDriver();
   auto affinity = internal::pinThreadAffinity();
   auto error = IOSError::OK;

   error = internal::ipcDriverAllocateRequest(ipcDriver,
                                              &ipcRequest,
                                              0,
                                              IOSCommand::Open,
                                              0,
                                              nullptr,
                                              nullptr);

   if (error < IOSError::OK) {
      goto fail;
   }

   error = internal::ipcPrepareOpenRequest(ipcDriver,
                                           ipcRequest,
                                           device,
                                           mode);

   if (error < IOSError::OK) {
      goto fail;
   }

   error = internal::ipcDriverSubmitRequest(ipcDriver, ipcRequest);

   if (error < IOSError::OK) {
      goto fail;
   }

   error = internal::ipcDriverWaitResponse(ipcDriver, ipcRequest);
   ipcRequest = nullptr;

   if (error < IOSError::OK) {
      goto fail;
   }

   ipcDriver->iosOpenRequestSuccess++;
   internal::unpinThreadAffinity(affinity);
   return error;

fail:
   ipcDriver->iosOpenRequestFail++;

   if (ipcRequest) {
      internal::ipcDriverFreeRequest(ipcDriver, ipcRequest);
   }

   internal::unpinThreadAffinity(affinity);
   return error;
}


/**
 * Sends an IOS Open command over IPC and calls callback with the result.
 *
 * \return
 * Returns IOSError::OK on success or an IOSError code otherwise.
 */
IOSError
IOS_OpenAsync(const char *device,
              IOSOpenMode mode,
              IOSAsyncCallbackFn callback,
              void *context)
{
   IPCDriverRequest *ipcRequest = nullptr;
   auto ipcDriver = internal::getIPCDriver();
   auto affinity = internal::pinThreadAffinity();
   auto error = IOSError::OK;

   error = internal::ipcDriverAllocateRequest(ipcDriver,
                                              &ipcRequest,
                                              0,
                                              IOSCommand::Open,
                                              0,
                                              callback,
                                              context);

   if (error < IOSError::OK) {
      goto fail;
   }

   error = internal::ipcPrepareOpenRequest(ipcDriver,
                                           ipcRequest,
                                           device,
                                           mode);

   if (error < IOSError::OK) {
      goto fail;
   }

   error = internal::ipcDriverSubmitRequest(ipcDriver, ipcRequest);

   if (error < IOSError::OK) {
      goto fail;
   }

   ipcDriver->iosOpenAsyncRequestSubmitSuccess++;
   internal::unpinThreadAffinity(affinity);
   return error;

fail:
   ipcDriver->iosOpenAsyncRequestSubmitFail++;

   if (ipcRequest) {
      internal::ipcDriverFreeRequest(ipcDriver, ipcRequest);
   }

   internal::unpinThreadAffinity(affinity);
   return error;
}


/**
 * Sends an IOS Close command over IPC and waits for the reply.
 *
 * \return
 * Returns IOSError::OK on success or an IOSError code otherwise.
 */
IOSError
IOS_Close(IOSHandle handle)
{
   IPCDriverRequest *ipcRequest = nullptr;
   auto ipcDriver = internal::getIPCDriver();
   auto affinity = internal::pinThreadAffinity();
   auto error = IOSError::OK;

   error = internal::ipcDriverAllocateRequest(ipcDriver,
                                              &ipcRequest,
                                              handle,
                                              IOSCommand::Close,
                                              0,
                                              nullptr,
                                              nullptr);

   if (error) {
      goto fail;
   }

   error = internal::ipcDriverSubmitRequest(ipcDriver, ipcRequest);

   if (error) {
      goto fail;
   }

   error = internal::ipcDriverWaitResponse(ipcDriver, ipcRequest);
   ipcRequest = nullptr;

   if (error) {
      goto fail;
   }

   ipcDriver->iosCloseRequestSuccess++;
   internal::unpinThreadAffinity(affinity);
   return IOSError::OK;

fail:
   ipcDriver->iosCloseRequestFail++;

   if (ipcRequest) {
      internal::ipcDriverFreeRequest(ipcDriver, ipcRequest);
   }

   internal::unpinThreadAffinity(affinity);
   return error;
}


/**
 * Sends an IOS Close command over IPC and calls callback with the result.
 *
 * \return
 * Returns IOSError::OK on success or an IOSError code otherwise.
 */
IOSError
IOS_CloseAsync(IOSHandle handle,
               IOSAsyncCallbackFn callback,
               void *context)
{
   IPCDriverRequest *ipcRequest = nullptr;
   auto ipcDriver = internal::getIPCDriver();
   auto affinity = internal::pinThreadAffinity();
   auto error = IOSError::OK;

   error = internal::ipcDriverAllocateRequest(ipcDriver,
                                              &ipcRequest,
                                              handle,
                                              IOSCommand::Close,
                                              0,
                                              callback,
                                              context);

   if (error) {
      goto fail;
   }

   error = internal::ipcDriverSubmitRequest(ipcDriver, ipcRequest);

   if (error) {
      goto fail;
   }

   ipcDriver->iosCloseAsyncRequestSubmitSuccess++;
   internal::unpinThreadAffinity(affinity);
   return IOSError::OK;

fail:
   ipcDriver->iosCloseAsyncRequestSubmitFail++;

   if (ipcRequest) {
      internal::ipcDriverFreeRequest(ipcDriver, ipcRequest);
   }

   internal::unpinThreadAffinity(affinity);
   return error;
}


/**
 * Sends an IOS Ioctl command over IPC and waits for the reply.
 *
 * \return
 * Returns IOSError::OK on success or an IOSError code otherwise.
 */
IOSError
IOS_Ioctl(IOSHandle handle,
          uint32_t request,
          void *inBuf,
          uint32_t inLen,
          void *outBuf,
          uint32_t outLen)
{
   IPCDriverRequest *ipcRequest = nullptr;
   auto ipcDriver = internal::getIPCDriver();
   auto affinity = internal::pinThreadAffinity();
   auto error = IOSError::OK;

   error = internal::ipcDriverAllocateRequest(ipcDriver,
                                              &ipcRequest,
                                              handle,
                                              IOSCommand::Ioctl,
                                              0,
                                              nullptr,
                                              nullptr);

   if (error) {
      goto fail;
   }

   error = internal::ipcPrepareIoctlRequest(ipcDriver,
                                            ipcRequest,
                                            request,
                                            inBuf,
                                            inLen,
                                            outBuf,
                                            outLen);

   if (error) {
      goto fail;
   }

   error = internal::ipcDriverSubmitRequest(ipcDriver, ipcRequest);

   if (error) {
      goto fail;
   }

   error = internal::ipcDriverWaitResponse(ipcDriver, ipcRequest);
   ipcRequest = nullptr;

   if (error) {
      goto fail;
   }

   ipcDriver->iosIoctlRequestSuccess++;
   internal::unpinThreadAffinity(affinity);
   return IOSError::OK;

fail:
   ipcDriver->iosIoctlRequestFail++;

   if (ipcRequest) {
      internal::ipcDriverFreeRequest(ipcDriver, ipcRequest);
   }

   internal::unpinThreadAffinity(affinity);
   return error;
}


/**
 * Sends an IOS Ioctl command over IPC and calls callback with the result.
 *
 * \return
 * Returns IOSError::OK on success or an IOSError code otherwise.
 */
IOSError
IOS_IoctlAsync(IOSHandle handle,
               uint32_t request,
               void *inBuf,
               uint32_t inLen,
               void *outBuf,
               uint32_t outLen,
               IOSAsyncCallbackFn callback,
               void *context)
{
   IPCDriverRequest *ipcRequest = nullptr;
   auto ipcDriver = internal::getIPCDriver();
   auto affinity = internal::pinThreadAffinity();
   auto error = IOSError::OK;

   error = internal::ipcDriverAllocateRequest(ipcDriver,
                                              &ipcRequest,
                                              handle,
                                              IOSCommand::Ioctl,
                                              0,
                                              callback,
                                              context);

   if (error) {
      goto fail;
   }

   error = internal::ipcPrepareIoctlRequest(ipcDriver,
                                            ipcRequest,
                                            request,
                                            inBuf,
                                            inLen,
                                            outBuf,
                                            outLen);

   if (error) {
      goto fail;
   }

   error = internal::ipcDriverSubmitRequest(ipcDriver, ipcRequest);

   if (error) {
      goto fail;
   }

   ipcDriver->iosIoctlAsyncRequestSubmitSuccess++;
   internal::unpinThreadAffinity(affinity);
   return IOSError::OK;

fail:
   ipcDriver->iosIoctlAsyncRequestSubmitFail++;

   if (ipcRequest) {
      internal::ipcDriverFreeRequest(ipcDriver, ipcRequest);
   }

   internal::unpinThreadAffinity(affinity);
   return error;
}


/**
 * Sends an IOS Ioctlv command over IPC and waits for the reply.
 *
 * \return
 * Returns IOSError::OK on success or an IOSError code otherwise.
 */
IOSError
IOS_Ioctlv(IOSHandle handle,
           uint32_t request,
           uint32_t vecIn,
           uint32_t vecOut,
           IOSVec *vec)
{
   IPCDriverRequest *ipcRequest = nullptr;
   auto ipcDriver = internal::getIPCDriver();
   auto affinity = internal::pinThreadAffinity();
   auto error = IOSError::OK;

   error = internal::ipcDriverAllocateRequest(ipcDriver,
                                              &ipcRequest,
                                              handle,
                                              IOSCommand::Ioctlv,
                                              0,
                                              nullptr,
                                              nullptr);

   if (error) {
      goto fail;
   }

   error = internal::ipcPrepareIoctlvRequest(ipcDriver,
                                             ipcRequest,
                                             request,
                                             vecIn,
                                             vecOut,
                                             vec);

   if (error) {
      goto fail;
   }

   error = internal::ipcDriverSubmitRequest(ipcDriver, ipcRequest);

   if (error) {
      goto fail;
   }

   error = internal::ipcDriverWaitResponse(ipcDriver, ipcRequest);
   ipcRequest = nullptr;

   if (error) {
      goto fail;
   }

   ipcDriver->iosIoctlvRequestSuccess++;
   internal::unpinThreadAffinity(affinity);
   return IOSError::OK;

fail:
   ipcDriver->iosIoctlvRequestFail++;

   if (ipcRequest) {
      internal::ipcDriverFreeRequest(ipcDriver, ipcRequest);
   }

   internal::unpinThreadAffinity(affinity);
   return error;
}


/**
 * Sends an IOS Ioctlv command over IPC and calls callback with the result.
 *
 * \return
 * Returns IOSError::OK on success or an IOSError code otherwise.
 */
IOSError
IOS_IoctlvAsync(IOSHandle handle,
                uint32_t request,
                uint32_t vecIn,
                uint32_t vecOut,
                IOSVec *vec,
                IOSAsyncCallbackFn callback,
                void *context)
{
   IPCDriverRequest *ipcRequest = nullptr;
   auto ipcDriver = internal::getIPCDriver();
   auto affinity = internal::pinThreadAffinity();
   auto error = IOSError::OK;

   error = internal::ipcDriverAllocateRequest(ipcDriver,
                                              &ipcRequest,
                                              handle,
                                              IOSCommand::Ioctlv,
                                              0,
                                              callback,
                                              context);

   if (error) {
      goto fail;
   }

   error = internal::ipcPrepareIoctlvRequest(ipcDriver,
                                             ipcRequest,
                                             request,
                                             vecIn,
                                             vecOut,
                                             vec);

   if (error) {
      goto fail;
   }

   error = internal::ipcDriverSubmitRequest(ipcDriver, ipcRequest);

   if (error) {
      goto fail;
   }

   ipcDriver->iosIoctlvAsyncRequestSubmitSuccess++;
   internal::unpinThreadAffinity(affinity);
   return IOSError::OK;

fail:
   ipcDriver->iosIoctlvAsyncRequestSubmitFail++;

   if (ipcRequest) {
      internal::ipcDriverFreeRequest(ipcDriver, ipcRequest);
   }

   internal::unpinThreadAffinity(affinity);
   return error;
}


void
Module::registerIosFunctions()
{
   RegisterKernelFunction(IOS_Open);
   RegisterKernelFunction(IOS_OpenAsync);
   RegisterKernelFunction(IOS_Close);
   RegisterKernelFunction(IOS_CloseAsync);
   RegisterKernelFunction(IOS_Ioctl);
   RegisterKernelFunction(IOS_IoctlAsync);
   RegisterKernelFunction(IOS_Ioctlv);
   RegisterKernelFunction(IOS_IoctlvAsync);
}


namespace internal
{

/**
 * Prepares an IPCDriverRequest structure with the parameters for IOS_Open.
 *
 * \retval IOSError::Max
 * The name of the device is too long.
 *
 * \retval IOSError::OK
 * Success.
 */
IOSError
ipcPrepareOpenRequest(IPCDriver *ipcDriver,
                      IPCDriverRequest *ipcRequest,
                      const char *device,
                      int mode)
{
   auto ipcBuffer = ipcRequest->ipcBuffer;
   auto deviceLen = strlen(device);

   if (deviceLen >= 0x20) {
      return IOSError::Max;
   }

   std::memset(ipcBuffer->nameBuffer, 0, 0x20);
   std::memcpy(ipcBuffer->nameBuffer, device, deviceLen);

   ipcBuffer->args[0] = 0;
   ipcBuffer->args[1] = deviceLen + 1;
   ipcBuffer->args[2] = mode;
   ipcBuffer->buffer1 = ipcBuffer->nameBuffer;
   return IOSError::OK;
}

/**
 * Prepares an IPCDriverRequest structure with the parameters for IOS_Ioctl.
 *
 * \retval IOSError::OK
 * Success.
 */
IOSError
ipcPrepareIoctlRequest(IPCDriver *ipcDriver,
                       IPCDriverRequest *ipcRequest,
                       uint32_t ioctlRequest,
                       void *inBuf,
                       uint32_t inLen,
                       void *outBuf,
                       uint32_t outLen)
{
   auto ipcBuffer = ipcRequest->ipcBuffer;
   ipcBuffer->args[0] = ioctlRequest;
   ipcBuffer->args[1] = 0;
   ipcBuffer->args[2] = inLen;
   ipcBuffer->args[3] = 0;
   ipcBuffer->args[4] = outLen;
   ipcBuffer->buffer1 = inBuf;
   ipcBuffer->buffer2 = outBuf;
   return IOSError::OK;
}


/**
 * Prepares an IPCDriverRequest structure with the parameters for IOS_Ioctlv.
 *
 * \retval IOSError::InvalidArg
 * One of the IOSVec structures has a NULL physical address.
 *
 * \retval IOSError::OK
 * Success.
 */
IOSError
ipcPrepareIoctlvRequest(IPCDriver *ipcDriver,
                        IPCDriverRequest *ipcRequest,
                        uint32_t ioctlvRequest,
                        uint32_t vecIn,
                        uint32_t vecOut,
                        IOSVec *vec)
{
   auto ipcBuffer = ipcRequest->ipcBuffer;
   ipcBuffer->args[0] = ioctlvRequest;
   ipcBuffer->args[1] = vecIn;
   ipcBuffer->args[2] = vecOut;
   ipcBuffer->args[3] = mem::untranslate(vec);
   ipcBuffer->buffer1 = reinterpret_cast<void *>(vec);

   for (auto i = 0u; i < vecIn + vecOut; ++i) {
      if (!vec[i].paddr && vec[i].len) {
         return IOSError::InvalidArg;
      }

      vec[i].vaddr = vec[i].paddr;
      vec[i].paddr = nullptr;
   }

   return IOSError::OK;
}

} // namespace internal

} // namespace coreinit
