#include "ios_fs_fsa.h"

namespace ios::fs
{

FSAStatus
FSADevice::getCwd(FSAResponseGetCwd &response)
{
   std::strncpy(response.path.phys_data().getRawPointer(),
                mWorkingPath.path().c_str(),
                response.path.size() - 1);
   return FSAStatus::OK;
}

} // namespace ios::fs
