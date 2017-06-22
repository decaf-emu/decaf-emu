#include "usr_cfg_device.h"
#include "kernel/kernel.h"

namespace kernel
{

namespace ios
{

namespace usr_cfg
{

IOSError
UserConfigDevice::open(IOSOpenMode mode)
{
   return IOSError::OK;
}


IOSError
UserConfigDevice::close()
{
   return IOSError::OK;
}


IOSError
UserConfigDevice::read(void *buffer,
                       size_t length)
{
   return IOSError::Invalid;
}


IOSError
UserConfigDevice::write(void *buffer,
                        size_t length)
{
   return IOSError::Invalid;
}


IOSError
UserConfigDevice::ioctl(uint32_t cmd,
                        void *inBuf,
                        size_t inLen,
                        void *outBuf,
                        size_t outLen)
{
   auto request = reinterpret_cast<UCRequest *>(inBuf);
   auto result = UCError::OK;

   switch (static_cast<UCCommand>(cmd)) {
   case UCCommand::ReadSysConfig:
      result = readSysConfig(&request->readSysConfigRequest);
      break;
   case UCCommand::WriteSysConfig:
      result = writeSysConfig(&request->writeSysConfigRequest);
      break;
   default:
      result = UCError::Error;
   }

   return static_cast<IOSError>(result);
}


IOSError
UserConfigDevice::ioctlv(uint32_t cmd,
                         size_t vecIn,
                         size_t vecOut,
                         IOSVec *vec)
{
   return IOSError::Invalid;
}


UCError
UserConfigDevice::readSysConfig(UCReadSysConfigRequest *request)
{
   return UCError::OK;
}


UCError
UserConfigDevice::writeSysConfig(UCWriteSysConfigRequest *request)
{
   return UCError::OK;
}

} // namespace usr_cfg

} // namespace ios

} // namespace kernel
