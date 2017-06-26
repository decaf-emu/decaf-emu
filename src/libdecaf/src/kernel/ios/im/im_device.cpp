#include "im_device.h"
#include "kernel/kernel.h"

namespace kernel
{

namespace ios
{

namespace im
{

IOSError
IMDevice::open(IOSOpenMode mode)
{
   return IOSError::OK;
}


IOSError
IMDevice::close()
{
   return IOSError::OK;
}


IOSError
IMDevice::read(void *buffer,
               size_t length)
{
   return IOSError::Invalid;
}


IOSError
IMDevice::write(void *buffer,
                size_t length)
{
   return IOSError::Invalid;
}


IOSError
IMDevice::ioctl(uint32_t cmd,
                void *inBuf,
                size_t inLen,
                void *outBuf,
                size_t outLen)
{
   return IOSError::Invalid;
}


IOSError
IMDevice::ioctlv(uint32_t cmd,
                 size_t vecIn,
                 size_t vecOut,
                 IOSVec *vec)
{
   auto result = IMError::OK;

   switch (static_cast<IMCommand>(cmd)) {
   case IMCommand::SetNvParameter:
   {
      decaf_check(vecIn == 0);
      decaf_check(vecOut == 1);
      decaf_check(vec[0].len == sizeof(IMSetNvParameterRequest));
      auto request = cpu::PhysicalPointer<IMSetNvParameterRequest> { vec[0].paddr };
      result = setNvParameter(request.getRawPointer());
      break;
   }
   case IMCommand::SetParameter:
   case IMCommand::GetParameter:
   case IMCommand::GetHomeButtonParams:
   case IMCommand::GetTimerRemaining:
   case IMCommand::GetNvParameter:
      break;
   default:
      result = static_cast<IMError>(IOSError::Invalid);
   }

   return static_cast<IOSError>(result);
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
      result = static_cast<IMError>(IOSError::InvalidArg);
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
      result = static_cast<IMError>(IOSError::InvalidArg);
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
      result = static_cast<IMError>(IOSError::InvalidArg);
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
      result = static_cast<IMError>(IOSError::InvalidArg);
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
      result = static_cast<IMError>(IOSError::InvalidArg);
   }

   return result;
}

} // namespace im

} // namespace ios

} // namespace kernel
