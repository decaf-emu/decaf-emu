#include "ios_auxil_usr_cfg_device.h"
#include "ios_auxil_usr_cfg_service_thread.h"

namespace ios::auxil::internal
{

void
UCDevice::setCloseRequest(phys_ptr<kernel::ResourceRequest> closeRequest)
{
   mCloseRequest = closeRequest;
}

void
UCDevice::incrementRefCount()
{
   mRefCount++;
}

void
UCDevice::decrementRefCount()
{
   mRefCount--;

   if (mRefCount == 0) {
      if (mCloseRequest) {
         kernel::IOS_ResourceReply(mCloseRequest, Error::OK);
      }

      mCloseRequest = nullptr;
      destroyUCDevice(this);
   }
}

UCError
UCDevice::readSysConfig(phys_ptr<UCReadSysConfigRequest> request)
{
   return UCError::Opcode;
}

UCError
UCDevice::writeSysConfig(phys_ptr<UCWriteSysConfigRequest> request)
{
   return UCError::Opcode;
}

} // namespace ios::auxil::internal
