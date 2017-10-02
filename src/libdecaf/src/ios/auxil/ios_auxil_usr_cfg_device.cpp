#include "ios_auxil_config.h"
#include "ios_auxil_usr_cfg_device.h"
#include "ios_auxil_usr_cfg_fs.h"
#include "ios_auxil_usr_cfg_service_thread.h"

#include <array>
#include <common/strutils.h>
#include <fmt/format.h>
#include <pugixml.hpp>
#include <sstream>
#include <string_view>

namespace ios::auxil::internal
{

static std::array<const char *, 66>
sValidRootKeys =
{
   "AVMCfg",
   "DRCCfg",
   "NETCfg0",
   "NETCfg1",
   "NETCfg2",
   "NETCfg3",
   "NETCfg4",
   "NETCfg5",
   "NETCmn",
   "audio",
   "avm_cdc",
   "avmflg",
   "avmStat",
   "btStd",
   "cafe",
   "ccr",
   "ccr_all",
   "coppa",
   "cos",
   "dc_state",
   "im_cfg",
   "nn",
   "nn_ram",
   "hbprefs",
   "p_acct1",
   "p_acct10",
   "p_acct11",
   "p_acct12",
   "p_acct2",
   "p_acct3",
   "p_acct4",
   "p_acct5",
   "p_acct6",
   "p_acct7",
   "p_acct8",
   "p_acct9",
   "parent",
   "PCFSvTCP",
   "console",
   "rmtCfg",
   "rootflg",
   "spotpass",
   "tvecfg",
   "tveEDID",
   "tveHdmi",
   "uvdflag",
   "wii_acct",
   "ums",
   "testProc",
   "clipbd",
   "hdmiEDID",
   "caffeine",
   "DRCDev",
   "hai_sys",
   "s_acct01",
   "s_acct02",
   "s_acct03",
   "s_acct04",
   "s_acct05",
   "s_acct06",
   "s_acct07",
   "s_acct08",
   "s_acct09",
   "s_acct10",
   "s_acct11",
   "s_acct12",
};

static bool
isValidRootKey(std::string_view key)
{
   for (auto validKey : sValidRootKeys) {
      if (key.compare(validKey) == 0) {
         return true;
      }
   }

   return false;
}

static std::string_view
getFileSysPath(UCFileSys fileSys)
{
   switch (fileSys) {
   case UCFileSys::Sys:
      return "/vol/sys/proc/prefs/";
   case UCFileSys::Slc:
      return "/vol/sys/proc_slc/prefs/";
   case UCFileSys::Ram:
      return "/vol/sys/proc_ram/prefs/";
   default:
      return "*error*";
   }
}

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
UCDevice::readSysConfig(uint32_t numVecIn,
                        phys_ptr<IoctlVec> vecs)
{
   auto request = phys_ptr<UCReadSysConfigRequest> { vecs[0].paddr };
   auto items = phys_cast<UCItem>(phys_addrof(request->settings[0]));

   if (request->count == 0) {
      return UCError::OK;
   }

   auto name = std::string_view { phys_addrof(request->settings[0].name).getRawPointer() };
   auto fileSys = getFileSys(name);
   if (fileSys == UCFileSys::Invalid) {
      return UCError::InvalidLocation;
   }

   auto rootKey = getRootKey(name);
   if (!isValidRootKey(rootKey)) {
      return UCError::FileSysName;
   }

   return readItems(getFileSysPath(fileSys), items, request->count, vecs);
}

UCError
UCDevice::writeSysConfig(uint32_t numVecIn,
                         phys_ptr<IoctlVec> vecs)
{
   auto request = phys_ptr<UCWriteSysConfigRequest> { vecs[0].paddr };
   auto items = phys_cast<UCItem>(phys_addrof(request->settings[0]));

   if (request->count == 0) {
      return UCError::OK;
   }

   auto name = std::string_view { phys_addrof(request->settings[0].name).getRawPointer() };
   auto fileSys = getFileSys(name);
   if (fileSys == UCFileSys::Invalid) {
      return UCError::InvalidLocation;
   }

   auto rootKey = getRootKey(name);
   if (!isValidRootKey(rootKey)) {
      return UCError::FileSysName;
   }

   return writeItems(getFileSysPath(fileSys), items, request->count, vecs);
}

} // namespace ios::auxil::internal
