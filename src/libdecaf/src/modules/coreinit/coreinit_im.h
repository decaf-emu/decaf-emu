#pragma once
#include "coreinit_ios.h"
#include "kernel/kernel_enum.h"

#include <cstdint>
#include <common/be_val.h>
#include <common/be_ptr.h>
#include <common/cbool.h>
#include <common/structsize.h>

namespace coreinit
{

/**
 * \defgroup coreinit_im IM
 * \ingroup coreinit
 * @{
 */

using IMCommand = kernel::IMCommand;
using IMError = kernel::IMError;
using IMParameter = kernel::IMParameter;
using IMTimer = kernel::IMTimer;

struct IMGetHomeButtonParamResponse
{
   be_val<uint32_t> unknown0x00;
   be_val<uint32_t> unknown0x04;
};
CHECK_OFFSET(IMGetHomeButtonParamResponse, 0x00, unknown0x00);
CHECK_OFFSET(IMGetHomeButtonParamResponse, 0x04, unknown0x04);

struct IMGetParameterRequest
{
   be_val<IMParameter> parameter;
};
CHECK_OFFSET(IMGetParameterRequest, 0x00, parameter);

struct IMGetParameterResponse
{
   be_val<IMParameter> parameter;
   be_val<uint32_t> value;
};
CHECK_OFFSET(IMGetParameterResponse, 0x00, parameter);
CHECK_OFFSET(IMGetParameterResponse, 0x04, value);

struct IMGetNvParameterRequest
{
   be_val<IMParameter> parameter;
};
CHECK_OFFSET(IMGetNvParameterRequest, 0x00, parameter);

struct IMGetNvParameterResponse
{
   be_val<IMParameter> parameter;
   be_val<uint32_t> value;
};
CHECK_OFFSET(IMGetNvParameterResponse, 0x00, parameter);
CHECK_OFFSET(IMGetNvParameterResponse, 0x04, value);

struct IMGetTimerRemainingRequest
{
   be_val<IMTimer> timer;
};
CHECK_OFFSET(IMGetTimerRemainingRequest, 0x00, timer);

struct IMGetTimerRemainingResponse
{
   be_val<IMTimer> timer;
   be_val<uint32_t> value;
};
CHECK_OFFSET(IMGetTimerRemainingResponse, 0x00, timer);
CHECK_OFFSET(IMGetTimerRemainingResponse, 0x04, value);

struct IMSetParameterRequest
{
   be_val<IMParameter> parameter;
   be_val<uint32_t> value;
};
CHECK_OFFSET(IMSetParameterRequest, 0x00, parameter);
CHECK_OFFSET(IMSetParameterRequest, 0x04, value);

struct IMSetNvParameterRequest
{
   be_val<IMParameter> parameter;
   be_val<uint32_t> value;
};
CHECK_OFFSET(IMSetNvParameterRequest, 0x00, parameter);
CHECK_OFFSET(IMSetNvParameterRequest, 0x04, value);

struct IMParameters
{
   be_val<uint32_t> unknown0x00;
   be_val<uint32_t> dimEnabled;
   be_val<uint32_t> dimPeriod;
   be_val<uint32_t> apdEnabled;
   be_val<uint32_t> apdPeriod;
};
CHECK_OFFSET(IMParameters, 0x00, unknown0x00);
CHECK_OFFSET(IMParameters, 0x04, dimEnabled);
CHECK_OFFSET(IMParameters, 0x08, dimPeriod);
CHECK_OFFSET(IMParameters, 0x0C, apdEnabled);
CHECK_OFFSET(IMParameters, 0x10, apdPeriod);

struct IMRequest
{
   union
   {
      IMGetNvParameterRequest getNvParameterRequest;
      IMGetNvParameterResponse getNvParameterResponse;
      IMGetParameterRequest getParameterRequest;
      IMGetParameterResponse getParameterResponse;
      IMGetHomeButtonParamResponse getHomeButtomParamResponse;
      IMSetParameterRequest setParameterRequest;
      IMSetNvParameterRequest setNvParameterRequest;
      IMGetTimerRemainingRequest getTimerRemainingRequest;
      IMGetTimerRemainingResponse getTimerRemainingResponse;
      uint8_t args[0x80];
   };

   IOSVec ioctlVecs[2];
   be_val<IOSHandle> handle;
   be_val<IMCommand> request;
   IOSAsyncCallbackFn asyncCallback;
   be_ptr<void> asyncCallbackContext;
   be_ptr<void> copySrc;
   be_ptr<void> copyDst;
   be_val<uint32_t> copySize;
};
CHECK_OFFSET(IMRequest, 0x80, ioctlVecs);
CHECK_OFFSET(IMRequest, 0x98, handle);
CHECK_OFFSET(IMRequest, 0x9C, request);
CHECK_OFFSET(IMRequest, 0xA0, asyncCallback);
CHECK_OFFSET(IMRequest, 0xA4, asyncCallbackContext);
CHECK_OFFSET(IMRequest, 0xA8, copySrc);
CHECK_OFFSET(IMRequest, 0xAC, copyDst);
CHECK_OFFSET(IMRequest, 0xB0, copySize);

IMError
IM_Open();

IMError
IM_Close(IOSHandle handle);

IMError
IM_GetHomeButtonParams(IOSHandle handle,
                       IMRequest *request,
                       void *output,
                       IOSAsyncCallbackFn asyncCallback,
                       void *asyncCallbackContext);

IMError
IM_GetParameter(IOSHandle handle,
                IMRequest *request,
                IMParameter parameter,
                void *output,
                IOSAsyncCallbackFn asyncCallback,
                void *asyncCallbackContext);

IMError
IM_GetParameters(IMParameters *parameters);

IMError
IM_GetNvParameter(IOSHandle handle,
                  IMRequest *request,
                  IMParameter parameter,
                  void *output,
                  IOSAsyncCallbackFn asyncCallback,
                  void *asyncCallbackContext);

IMError
IM_GetNvParameterWithoutHandleAndItb(IMParameter parameter,
                                     be_val<uint32_t> *value);

IMError
IM_GetRuntimeParameter(IMParameter parameter,
                       be_val<uint32_t> *value);

IMError
IM_GetTimerRemaining(IOSHandle handle,
                     IMRequest *request,
                     IMTimer timer,
                     void *output,
                     IOSAsyncCallbackFn asyncCallback,
                     void *asyncCallbackContext);

IMError
IM_GetTimerRemainingSeconds(IMTimer timer,
                            be_val<uint32_t> *seconds);

IMError
IM_SetParameter(IOSHandle handle,
                IMRequest *request,
                IMParameter parameter,
                uint32_t value,
                IOSAsyncCallbackFn asyncCallback,
                void *asyncCallbackContext);

IMError
IM_SetRuntimeParameter(IMParameter parameter,
                       uint32_t value);

IMError
IMDisableAPD();

IMError
IMDisableDim();

IMError
IMEnableAPD();

IMError
IMEnableDim();

IMError
IMIsAPDEnabled(be_val<uint32_t> *value);

IMError
IMIsAPDEnabledBySysSettings(be_val<uint32_t> *value);

IMError
IMIsDimEnabled(be_val<uint32_t> *value);

IMError
IMGetDimEnableDrc(be_val<uint32_t> *value);

IMError
IMGetDimEnableTv(be_val<uint32_t> *value);

IMError
IMGetDimPeriod(be_val<uint32_t> *value);

IMError
IMGetTimeBeforeAPD(be_val<uint32_t> *seconds);

IMError
IMGetTimeBeforeDimming(be_val<uint32_t> *seconds);

IMError
IMSetDimEnableDrc(BOOL value);

IMError
IMSetDimEnableTv(BOOL value);

IMError
IMStartAPDVideoMode();

/** @} */

} // namespace coreinit
