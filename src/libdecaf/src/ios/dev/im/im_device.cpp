#include "im_device.h"
#include "kernel/kernel.h"

namespace ios
{

namespace dev
{

namespace im
{

Error
IMDevice::open(OpenMode mode)
{
   return Error::OK;
}


Error
IMDevice::close()
{
   return Error::OK;
}


Error
IMDevice::read(void *buffer,
               size_t length)
{
   return Error::Invalid;
}


Error
IMDevice::write(void *buffer,
                size_t length)
{
   return Error::Invalid;
}


Error
IMDevice::ioctl(uint32_t cmd,
                void *inBuf,
                size_t inLen,
                void *outBuf,
                size_t outLen)
{
   return Error::Invalid;
}


Error
IMDevice::ioctlv(uint32_t cmd,
                 size_t vecIn,
                 size_t vecOut,
                 IoctlVec *vec)
{
   auto result = IMError::OK;

   switch (static_cast<IMCommand>(cmd)) {
   case IMCommand::SetNvParameter:
   {
      decaf_check(vecIn == 1);
      decaf_check(vecOut == 0);
      decaf_check(vec[0].len == sizeof(IMSetNvParameterRequest));
      auto request = cpu::PhysicalPointer<IMSetNvParameterRequest> { vec[0].paddr };
      result = setNvParameter(request.getRawPointer());
      break;
   }
   case IMCommand::SetParameter:
   {
      decaf_check(vecIn == 1);
      decaf_check(vecOut == 0);
      decaf_check(vec[0].len == sizeof(IMSetParameterRequest));
      auto request = cpu::PhysicalPointer<IMSetParameterRequest> { vec[0].paddr };
      result = setParameter(request.getRawPointer());
      break;
   }
   case IMCommand::GetParameter:
   {
      decaf_check(vecIn == 1);
      decaf_check(vecOut == 1);
      decaf_check(vec[0].len == sizeof(IMGetParameterRequest));
      decaf_check(vec[1].len == sizeof(IMGetParameterResponse));
      auto request = cpu::PhysicalPointer<IMGetParameterRequest> { vec[0].paddr };
      auto response = cpu::PhysicalPointer<IMGetParameterResponse> { vec[1].paddr };
      result = getParameter(request.getRawPointer(), response.getRawPointer());
      break;
   }
   case IMCommand::GetNvParameter:
   {
      decaf_check(vecIn == 1);
      decaf_check(vecOut == 1);
      decaf_check(vec[0].len == sizeof(IMGetNvParameterRequest));
      decaf_check(vec[1].len == sizeof(IMGetNvParameterResponse));
      auto request = cpu::PhysicalPointer<IMGetNvParameterRequest> { vec[0].paddr };
      auto response = cpu::PhysicalPointer<IMGetNvParameterResponse> { vec[1].paddr };
      result = getNvParameter(request.getRawPointer(), response.getRawPointer());
      break;
   }
   case IMCommand::GetHomeButtonParams:
   {
      decaf_check(vecIn == 0);
      decaf_check(vecOut == 1);
      decaf_check(vec[0].len == sizeof(IMGetHomeButtonParamResponse));
      auto response = cpu::PhysicalPointer<IMGetHomeButtonParamResponse> { vec[0].paddr };
      result = getHomeButtonParams(response.getRawPointer());
      break;
   }
   case IMCommand::GetTimerRemaining:
   {
      decaf_check(vecIn == 1);
      decaf_check(vecOut == 1);
      decaf_check(vec[0].len == sizeof(IMGetTimerRemainingRequest));
      decaf_check(vec[1].len == sizeof(IMGetTimerRemainingResponse));
      auto request = cpu::PhysicalPointer<IMGetTimerRemainingRequest> { vec[0].paddr };
      auto response = cpu::PhysicalPointer<IMGetTimerRemainingResponse> { vec[1].paddr };
      result = getTimerRemaining(request.getRawPointer(), response.getRawPointer());
      break;
   }
   default:
      result = static_cast<IMError>(Error::Invalid);
   }

   return static_cast<Error>(result);
}


IMError
IMDevice::getHomeButtonParams(IMGetHomeButtonParamResponse *response)
{
   response->unknown0x00 = 0;
   response->unknown0x04 = 0;
   return IMError::OK;
}


IMError
IMDevice::getParameter(IMGetParameterRequest *request,
                       IMGetParameterResponse *response)
{
   auto result = IMError::OK;

   switch (request->parameter) {
   case IMParameter::DimEnabled:
      response->value = mDimEnabled;
      break;
   case IMParameter::DimPeriod:
      response->value = mDimPeriodSeconds;
      break;
   case IMParameter::APDEnabled:
      response->value = mAPDEnabled;
      break;
   case IMParameter::APDPeriod:
      response->value = mAPDPeriodSeconds;
      break;
   case IMParameter::Unknown5:
      response->value = mUnknownParameter5;
      break;
   case IMParameter::DimEnableTv:
      response->value = mDimEnableTv;
      break;
   case IMParameter::DimEnableDrc:
      response->value = mDimEnableDrc;
      break;
   default:
      result = static_cast<IMError>(Error::InvalidArg);
   }

   return result;
}


IMError
IMDevice::getNvParameter(IMGetNvParameterRequest *request,
                         IMGetNvParameterResponse *response)
{
   auto result = IMError::OK;

   switch (request->parameter) {
   case IMParameter::DimEnabled:
      response->value = mDimEnabled;
      break;
   case IMParameter::DimPeriod:
      response->value = mDimPeriodSeconds;
      break;
   case IMParameter::APDEnabled:
      response->value = mAPDEnabled;
      break;
   case IMParameter::APDPeriod:
      response->value = mAPDPeriodSeconds;
      break;
   case IMParameter::Unknown5:
      response->value = mUnknownParameter5;
      break;
   case IMParameter::DimEnableTv:
      response->value = mDimEnableTv;
      break;
   case IMParameter::DimEnableDrc:
      response->value = mDimEnableDrc;
      break;
   default:
      result = static_cast<IMError>(Error::InvalidArg);
   }

   return result;
}


IMError
IMDevice::getTimerRemaining(IMGetTimerRemainingRequest *request,
                            IMGetTimerRemainingResponse *response)
{
   auto result = IMError::OK;

   switch (request->timer) {
   case IMTimer::APD:
      response->value = mAPDPeriodSeconds;
      break;
   case IMTimer::Dim:
      response->value = mDimPeriodSeconds;
      break;
   default:
      result = static_cast<IMError>(Error::InvalidArg);
   }

   return result;
}


IMError
IMDevice::setParameter(IMSetParameterRequest *request)
{
   auto result = IMError::OK;

   switch (request->parameter) {
   case IMParameter::DimEnabled:
      mDimEnabled = request->value;
      break;
   case IMParameter::DimPeriod:
      mDimPeriodSeconds = request->value;
      break;
   case IMParameter::APDEnabled:
      mAPDEnabled = request->value;
      break;
   case IMParameter::APDPeriod:
      mAPDPeriodSeconds = request->value;
      break;
   case IMParameter::Unknown5:
      mUnknownParameter5 = request->value;
      break;
   case IMParameter::DimEnableTv:
      mDimEnableTv = request->value;
      break;
   case IMParameter::DimEnableDrc:
      mDimEnableDrc = request->value;
      break;
   default:
      result = static_cast<IMError>(Error::InvalidArg);
   }

   return result;
}


IMError
IMDevice::setNvParameter(IMSetNvParameterRequest *request)
{
   auto result = IMError::OK;

   switch (request->parameter) {
   case IMParameter::DimEnabled:
      mDimEnabled = request->value;
      break;
   case IMParameter::DimPeriod:
      mDimPeriodSeconds = request->value;
      break;
   case IMParameter::APDEnabled:
      mAPDEnabled = request->value;
      break;
   case IMParameter::APDPeriod:
      mAPDPeriodSeconds = request->value;
      break;
   case IMParameter::Unknown5:
      mUnknownParameter5 = request->value;
      break;
   case IMParameter::DimEnableTv:
      mDimEnableTv = request->value;
      break;
   case IMParameter::DimEnableDrc:
      mDimEnableDrc = request->value;
      break;
   default:
      result = static_cast<IMError>(Error::InvalidArg);
   }

   return result;
}

} // namespace im

} // namespace dev

} // namespace ios
