#pragma once
#include "ios_auxil_usr_cfg_types.h"
#include "ios_auxil_usr_cfg_request.h"
#include "ios/kernel/ios_kernel_resourcemanager.h"

namespace ios::auxil::internal
{

class UCDevice
{
public:
   void
   setCloseRequest(phys_ptr<kernel::ResourceRequest> closeRequest);

   void
   incrementRefCount();

   void
   decrementRefCount();

   UCError
   readSysConfig(uint32_t numVecIn,
                 phys_ptr<IoctlVec> vecs);

   UCError
   writeSysConfig(uint32_t numVecIn,
                  phys_ptr<IoctlVec> vecs);

private:
   int mRefCount = 1;
   phys_ptr<kernel::ResourceRequest> mCloseRequest = nullptr;
};

} // namespace ios::auxil::internal
