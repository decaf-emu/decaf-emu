#include "coreinit.h"
#include "coreinit_im.h"
#include "coreinit_ios.h"
#include "coreinit_mutex.h"
#include "ppcutils/wfunc_call.h"
#include "ppcutils/stackobject.h"
#include <cstring>

namespace coreinit
{

namespace internal
{

static OSMutex *
sItbMutex = nullptr;

static IMRequest *
sSharedRequest = nullptr;

static IOSAsyncCallbackFn
sIosAsyncCallbackFn = nullptr;

static void
imAcquireItbMutex()
{
   OSLockMutex(sItbMutex);
}

static void
imReleaseItbMutex()
{
   OSUnlockMutex(sItbMutex);
}

static void
imCopyData(IMRequest *request)
{
   if (request->copyDst && request->copySrc && request->copySize) {
      std::memcpy(virt_addrof(request->copyDst).getRawPointer(),
                  virt_addrof(request->copySrc).getRawPointer(),
                  request->copySize);
   }
}

static void
imIosAsyncCallback(IOSError error,
                   void *context)
{
   auto request = reinterpret_cast<IMRequest *>(context);
   if (error == IOSError::OK) {
      imCopyData(request);
   }

   request->asyncCallback(error, request->asyncCallbackContext.getRawPointer());
}

static IMError
imSendRequest(IMRequest *request,
              uint32_t vecIn,
              uint32_t vecOut)
{
   IOSError error;

   if (request->asyncCallback) {
      error = IOS_IoctlvAsync(request->handle,
                              request->request,
                              vecIn,
                              vecOut,
                              virt_addrof(request->ioctlVecs).getRawPointer(),
                              sIosAsyncCallbackFn,
                              request);
   } else {
      error = IOS_Ioctlv(request->handle,
                         request->request,
                         vecIn,
                         vecOut,
                         virt_addrof(request->ioctlVecs).getRawPointer());

      if (vecOut > 0 && error == IOSError::OK) {
         imCopyData(request);
      }
   }

   return static_cast<IMError>(error);
}

} // namespace internal


IMError
IM_Open()
{
   return static_cast<IMError>(IOS_Open("/dev/im", IOSOpenMode::None));
}


IMError
IM_Close(IOSHandle handle)
{
   return static_cast<IMError>(IOS_Close(handle));
}


IMError
IM_GetHomeButtonParams(IOSHandle handle,
                       IMRequest *request,
                       void *output,
                       IOSAsyncCallbackFn asyncCallback,
                       void *asyncCallbackContext)
{
   std::memset(request, 0, sizeof(IMRequest));

   request->ioctlVecs[0].vaddr = virt_addrof(request->getHomeButtomParamResponse);
   request->ioctlVecs[0].len = 8u;

   request->handle = handle;
   request->request = IMCommand::GetHomeButtonParams;
   request->asyncCallback = asyncCallback;
   request->asyncCallbackContext = cpu::translate(asyncCallbackContext);
   request->copySrc = virt_addrof(request->getHomeButtomParamResponse);
   request->copyDst = cpu::translate(output);
   request->copySize = 8u;

   return internal::imSendRequest(request, 0, 1);
}


IMError
IM_GetParameter(IOSHandle handle,
                IMRequest *request,
                IMParameter parameter,
                void *output,
                IOSAsyncCallbackFn asyncCallback,
                void *asyncCallbackContext)
{
   std::memset(request, 0, sizeof(IMRequest));

   request->getParameterRequest.parameter = parameter;
   request->ioctlVecs[0].vaddr = virt_addrof(request->getParameterRequest);
   request->ioctlVecs[0].len = 8u;

   request->ioctlVecs[1].vaddr = virt_addrof(request->getParameterResponse);
   request->ioctlVecs[1].len = 8u;

   request->handle = handle;
   request->request = IMCommand::GetParameter;
   request->asyncCallback = asyncCallback;
   request->asyncCallbackContext = cpu::translate(asyncCallbackContext);
   request->copySrc = virt_addrof(request->getParameterResponse.value);
   request->copyDst = cpu::translate(output);
   request->copySize = 4u;

   return internal::imSendRequest(request, 1, 1);
}


IMError
IM_GetParameters(IMParameters *parameters)
{
   auto result = static_cast<IMError>(IOS_Open("/dev/im", IOSOpenMode::None));
   if (result < 0) {
      return static_cast<IMError>(result);
   }

   auto handle = static_cast<IOSHandle>(result);
   internal::imAcquireItbMutex();

   result = IM_GetParameter(handle,
                            internal::sSharedRequest,
                            IMParameter::ResetEnable,
                            virt_addrof(parameters->resetEnabled).getRawPointer(),
                            nullptr,
                            nullptr);
   if (result != IMError::OK) {
      goto out;
   }

   result = IM_GetParameter(handle,
                            internal::sSharedRequest,
                            IMParameter::DimEnabled,
                            virt_addrof(parameters->dimEnabled).getRawPointer(),
                            nullptr,
                            nullptr);
   if (result != IMError::OK) {
      goto out;
   }

   result = IM_GetParameter(handle,
                            internal::sSharedRequest,
                            IMParameter::DimPeriod,
                            virt_addrof(parameters->dimPeriod).getRawPointer(),
                            nullptr,
                            nullptr);
   if (result != IMError::OK) {
      goto out;
   }

   result = IM_GetParameter(handle,
                            internal::sSharedRequest,
                            IMParameter::APDEnabled,
                            virt_addrof(parameters->apdEnabled).getRawPointer(),
                            nullptr,
                            nullptr);
   if (result != IMError::OK) {
      goto out;
   }

   result = IM_GetParameter(handle,
                            internal::sSharedRequest,
                            IMParameter::APDPeriod,
                            virt_addrof(parameters->apdPeriod).getRawPointer(),
                            nullptr,
                            nullptr);
   if (result != IMError::OK) {
      goto out;
   }

out:
   internal::imReleaseItbMutex();
   IOS_Close(handle);
   return result;
}


IMError
IM_GetNvParameter(IOSHandle handle,
                  IMRequest *request,
                  IMParameter parameter,
                  void *output,
                  IOSAsyncCallbackFn asyncCallback,
                  void *asyncCallbackContext)
{
   std::memset(request, 0, sizeof(IMRequest));

   request->getNvParameterRequest.parameter = parameter;
   request->ioctlVecs[0].vaddr = virt_addrof(request->getNvParameterRequest);
   request->ioctlVecs[0].len = 8u;

   request->ioctlVecs[1].vaddr = virt_addrof(request->getNvParameterResponse);
   request->ioctlVecs[1].len = 8u;

   request->handle = handle;
   request->request = IMCommand::GetNvParameter;
   request->asyncCallback = asyncCallback;
   request->asyncCallbackContext = cpu::translate(asyncCallbackContext);
   request->copySrc = virt_addrof(request->getNvParameterResponse.value);
   request->copyDst = cpu::translate(output);
   request->copySize = 4u;

   return internal::imSendRequest(request, 1, 1);
}


IMError
IM_GetNvParameterWithoutHandleAndItb(IMParameter parameter,
                                     be_val<uint32_t> *value)
{
   auto result = static_cast<IMError>(IOS_Open("/dev/im", IOSOpenMode::None));
   if (result < 0) {
      return static_cast<IMError>(result);
   }

   auto handle = static_cast<IOSHandle>(result);
   internal::imAcquireItbMutex();
   result = IM_GetNvParameter(handle, internal::sSharedRequest, parameter, value, nullptr, nullptr);
   internal::imReleaseItbMutex();
   IOS_Close(handle);
   return result;
}


IMError
IM_GetRuntimeParameter(IMParameter parameter,
                       be_val<uint32_t> *value)
{
   auto result = static_cast<IMError>(IOS_Open("/dev/im", IOSOpenMode::None));
   if (result < 0) {
      return static_cast<IMError>(result);
   }

   auto handle = static_cast<IOSHandle>(result);
   internal::imAcquireItbMutex();
   result = IM_GetParameter(handle, internal::sSharedRequest, parameter, value, nullptr, nullptr);
   internal::imReleaseItbMutex();
   IOS_Close(handle);
   return result;
}


IMError
IM_GetTimerRemaining(IOSHandle handle,
                     IMRequest *request,
                     IMTimer timer,
                     void *output,
                     IOSAsyncCallbackFn asyncCallback,
                     void *asyncCallbackContext)
{
   std::memset(request, 0, sizeof(IMRequest));

   request->getTimerRemainingRequest.timer = timer;
   request->ioctlVecs[0].vaddr = virt_addrof(request->getTimerRemainingRequest);
   request->ioctlVecs[0].len = 8u;

   request->ioctlVecs[1].vaddr = virt_addrof(request->getTimerRemainingResponse);
   request->ioctlVecs[1].len = static_cast<uint32_t>(sizeof(IMGetTimerRemainingResponse));

   request->handle = handle;
   request->request = IMCommand::GetTimerRemaining;
   request->asyncCallback = asyncCallback;
   request->asyncCallbackContext = cpu::translate(asyncCallbackContext);
   request->copySrc = virt_addrof(request->getTimerRemainingResponse.value);
   request->copyDst = cpu::translate(output);
   request->copySize = 4u;

   return internal::imSendRequest(request, 1, 1);
}


IMError
IM_GetTimerRemainingSeconds(IMTimer timer,
                            be_val<uint32_t> *seconds)
{
   auto result = static_cast<IMError>(IOS_Open("/dev/im", IOSOpenMode::None));
   if (result < 0) {
      return static_cast<IMError>(result);
   }

   auto handle = static_cast<IOSHandle>(result);
   internal::imAcquireItbMutex();
   result = IM_GetTimerRemaining(handle, internal::sSharedRequest, timer, seconds, nullptr, nullptr);
   internal::imReleaseItbMutex();
   IOS_Close(handle);
   return result;
}


IMError
IM_SetParameter(IOSHandle handle,
                IMRequest *request,
                IMParameter parameter,
                uint32_t value,
                IOSAsyncCallbackFn asyncCallback,
                void *asyncCallbackContext)
{
   std::memset(request, 0, sizeof(IMRequest));

   request->setParameterRequest.parameter = parameter;
   request->setParameterRequest.value = value;
   request->ioctlVecs[0].vaddr = virt_addrof(request->setParameterRequest);
   request->ioctlVecs[0].len = 8u;

   request->handle = handle;
   request->request = IMCommand::SetParameter;
   request->asyncCallback = asyncCallback;
   request->asyncCallbackContext = cpu::translate(asyncCallbackContext);

   return internal::imSendRequest(request, 1, 0);
}


IMError
IM_SetNvParameter(IOSHandle handle,
                  IMRequest *request,
                  IMParameter parameter,
                  uint32_t value,
                  IOSAsyncCallbackFn asyncCallback,
                  void *asyncCallbackContext)
{
   std::memset(request, 0, sizeof(IMRequest));

   request->setNvParameterRequest.parameter = parameter;
   request->setNvParameterRequest.value = value;
   request->ioctlVecs[0].vaddr = virt_addrof(request->setNvParameterRequest);
   request->ioctlVecs[0].len = 8u;

   request->handle = handle;
   request->request = IMCommand::SetNvParameter;
   request->asyncCallback = asyncCallback;
   request->asyncCallbackContext = cpu::translate(asyncCallbackContext);

   return internal::imSendRequest(request, 1, 0);
}


IMError
IM_SetRuntimeParameter(IMParameter parameter,
                       uint32_t value)
{
   auto result = static_cast<IMError>(IOS_Open("/dev/im", IOSOpenMode::None));
   if (result < 0) {
      return static_cast<IMError>(result);
   }

   auto handle = static_cast<IOSHandle>(result);
   internal::imAcquireItbMutex();
   result = IM_SetParameter(handle, internal::sSharedRequest, parameter, value, nullptr, nullptr);
   internal::imReleaseItbMutex();
   IOS_Close(handle);
   return result;
}


IMError
IMDisableAPD()
{
   return IM_SetRuntimeParameter(IMParameter::APDEnabled, FALSE);
}


IMError
IMDisableDim()
{
   auto result = IM_SetRuntimeParameter(IMParameter::DimEnabled, FALSE);
   if (result != IMError::OK) {
      return result;
   }

   return IM_SetRuntimeParameter(IMParameter::ResetEnable, FALSE);
}


IMError
IMEnableAPD()
{
   ppcutils::StackObject<be_val<uint32_t>> previous;
   auto result = IM_GetNvParameterWithoutHandleAndItb(IMParameter::APDEnabled, previous);

   if (result != IMError::OK) {
      return result;
   }

   if (*previous == TRUE) {
      return IMError::OK;
   }

   return IM_SetRuntimeParameter(IMParameter::APDEnabled, TRUE);
}


IMError
IMEnableDim()
{
   ppcutils::StackObject<be_val<uint32_t>> previous;
   auto result = IM_GetNvParameterWithoutHandleAndItb(IMParameter::DimEnabled, previous);
   if (result != IMError::OK) {
      return result;
   }

   if (*previous == TRUE) {
      return IMError::OK;
   }

   result = IM_SetRuntimeParameter(IMParameter::DimEnabled, TRUE);
   if (result != IMError::OK) {
      return result;
   }

   result = IM_GetNvParameterWithoutHandleAndItb(IMParameter::ResetEnable, previous);
   if (result != IMError::OK) {
      return result;
   }

   if (*previous == TRUE) {
      return IMError::OK;
   }

   return IM_SetRuntimeParameter(IMParameter::ResetEnable, FALSE);
}


IMError
IMIsAPDEnabled(be_val<uint32_t> *value)
{
   return IM_GetRuntimeParameter(IMParameter::APDEnabled, value);
}


IMError
IMIsAPDEnabledBySysSettings(be_val<uint32_t> *value)
{
   return IM_GetNvParameterWithoutHandleAndItb(IMParameter::APDEnabled, value);
}


IMError
IMIsDimEnabled(be_val<uint32_t> *value)
{
   return IM_GetRuntimeParameter(IMParameter::DimEnabled, value);
}


IMError
IMGetAPDPeriod(be_val<uint32_t> *value)
{
   return IM_GetRuntimeParameter(IMParameter::APDPeriod, value);
}


IMError
IMGetDimEnableDrc(be_val<uint32_t> *value)
{
   return IM_GetRuntimeParameter(IMParameter::DimEnableDrc, value);
}


IMError
IMGetDimEnableTv(be_val<uint32_t> *value)
{
   return IM_GetRuntimeParameter(IMParameter::DimEnableTv, value);
}


IMError
IMGetDimPeriod(be_val<uint32_t> *value)
{
   return IM_GetRuntimeParameter(IMParameter::DimPeriod, value);
}


IMError
IMGetTimeBeforeAPD(be_val<uint32_t> *seconds)
{
   return IM_GetTimerRemainingSeconds(IMTimer::APD, seconds);
}


IMError
IMGetTimeBeforeDimming(be_val<uint32_t> *seconds)
{
   return IM_GetTimerRemainingSeconds(IMTimer::Dim, seconds);
}


IMError
IMSetDimEnableDrc(BOOL value)
{
   return IM_SetRuntimeParameter(IMParameter::DimEnableTv, value);
}


IMError
IMSetDimEnableTv(BOOL value)
{
   return IM_SetRuntimeParameter(IMParameter::DimEnableTv, value);
}


IMError
IMStartAPDVideoMode()
{
   ppcutils::StackObject<be_val<uint32_t>> previous;
   auto result = IM_GetNvParameterWithoutHandleAndItb(IMParameter::APDPeriod, previous);

   if (result != IMError::OK) {
      return result;
   }

   if (*previous == 14400) {
      return IMError::OK;
   }

   return IM_SetRuntimeParameter(IMParameter::APDPeriod, 14400);
}


void
Module::registerImFunctions()
{
   RegisterKernelFunction(IM_Open);
   RegisterKernelFunction(IM_Close);
   RegisterKernelFunction(IM_GetHomeButtonParams);
   RegisterKernelFunction(IM_GetParameter);
   RegisterKernelFunction(IM_GetParameters);
   RegisterKernelFunction(IM_GetNvParameter);
   RegisterKernelFunction(IM_GetNvParameterWithoutHandleAndItb);
   RegisterKernelFunction(IM_GetRuntimeParameter);
   RegisterKernelFunction(IM_GetTimerRemaining);
   RegisterKernelFunction(IM_GetTimerRemainingSeconds);
   RegisterKernelFunction(IM_SetParameter);
   RegisterKernelFunction(IM_SetRuntimeParameter);

   RegisterKernelFunction(IMDisableAPD);
   RegisterKernelFunction(IMDisableDim);
   RegisterKernelFunction(IMEnableAPD);
   RegisterKernelFunction(IMEnableDim);
   RegisterKernelFunction(IMIsAPDEnabled);
   RegisterKernelFunction(IMIsAPDEnabledBySysSettings);
   RegisterKernelFunction(IMIsDimEnabled);
   RegisterKernelFunction(IMGetAPDPeriod);
   RegisterKernelFunction(IMGetDimEnableDrc);
   RegisterKernelFunction(IMGetDimEnableTv);
   RegisterKernelFunction(IMGetDimPeriod);
   RegisterKernelFunction(IMGetTimeBeforeAPD);
   RegisterKernelFunction(IMGetTimeBeforeDimming);
   RegisterKernelFunction(IMSetDimEnableDrc);
   RegisterKernelFunction(IMSetDimEnableTv);
   RegisterKernelFunction(IMStartAPDVideoMode);

   RegisterInternalData(internal::sItbMutex);
   RegisterInternalData(internal::sSharedRequest);
   RegisterInternalFunction(internal::imIosAsyncCallback, internal::sIosAsyncCallbackFn);
}

} // namespace coreinit
