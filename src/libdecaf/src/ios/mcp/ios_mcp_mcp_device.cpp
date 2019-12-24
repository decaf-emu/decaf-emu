#include "ios_mcp_config.h"
#include "ios_mcp_enum.h"
#include "ios_mcp_mcp_device.h"
#include "ios_mcp_mcp_types.h"
#include "ios_mcp_mcp_request.h"
#include "ios_mcp_mcp_response.h"
#include "ios_mcp_mcp_thread.h"
#include "ios_mcp_title.h"

#include "cafe/libraries/cafe_hle.h"
#include "decaf_config.h"
#include "ios/ios.h"
#include "ios/ios_stackobject.h"
#include "ios/fs/ios_fs_fsa_ipc.h"
#include "ios/kernel/ios_kernel_process.h"
#include "ios/kernel/ios_kernel_resourcemanager.h"
#include "vfs/vfs_host_device.h"
#include "vfs/vfs_virtual_device.h"

#include <common/log.h>

namespace ios::mcp::internal
{

using namespace ios::fs;
using namespace ios::kernel;

static std::string sCurrentLoadFilePath = { };
static FSAHandle sCurrentLoadFileHandle = { };
static size_t sCurrentLoadFileSize = 0u;

MCPError
mcpDeviceList(phys_ptr<const MCPRequestDeviceList> request,
              phys_ptr<MCPDevice> deviceList,
              uint32_t deviceListSizeBytes)
{
   auto deviceListCount = deviceListSizeBytes / sizeof(MCPDevice);
   auto devicesAdded = 0;
   auto config = decaf::config();
   auto deviceId = 0u;

   /*
   Example device list from my console:
   path                    type      fs  flags  index uid
   /vol/storage_usb01,     usb,      wfs,  0xF,     1   7
   /vol/storage_bt00,      bt,       "",   0x3,     0   6
   /vol/storage_drh00,     drh,      "",   0x3,     0   5
   /vol/storage_slccmpt01, slccmpt,  isfs, 0x7,     1   4
   /vol/storage_slc01,     slc,      isfs, 0xF,     1   3
   /vol/storage_sdcard01,  sdcard,   fat,  0x7,     1   2
   /vol/storage_ramdisk01, ramdisk,  wfs,  0x7,     1   1
   /vol/storage_mlc01,     mlc,      wfs,  0xF,     1   0

   Only mlc/ramdisk/usb had unk0x08 set to some sort of 7 character unique
   identifier looking string.
   */

   if (devicesAdded < deviceListCount &&
       (request->flags & MCPDeviceFlags::Unk1) &&
       !config->system.mlc_path.empty()) {
      auto &device = deviceList[devicesAdded++];
      std::memset(std::addressof(device), 0, sizeof(MCPDevice));

      device.type = "mlc";
      device.filesystem = "wfs";
      device.flags = MCPDeviceFlags::Unk1 | MCPDeviceFlags::Unk2 | MCPDeviceFlags::Unk4 | MCPDeviceFlags::Unk8;
      device.index = 1u;
      device.path = "/vol/storage_mlc01";
      device.uid = deviceId++;
   }

   if (devicesAdded < deviceListCount &&
       (request->flags & MCPDeviceFlags::Unk1) &&
       !config->system.slc_path.empty()) {
      auto &device = deviceList[devicesAdded++];
      std::memset(std::addressof(device), 0, sizeof(MCPDevice));

      device.type = "slc";
      device.filesystem = "isfs";
      device.flags = MCPDeviceFlags::Unk1 | MCPDeviceFlags::Unk2 | MCPDeviceFlags::Unk4 | MCPDeviceFlags::Unk8;
      device.index = 1u;
      device.path = "/vol/storage_slc01";
      device.uid = deviceId++;
   }

   return static_cast<MCPError>(devicesAdded);
}

MCPError
mcpGetFileLength(phys_ptr<const MCPRequestGetFileLength> request)
{
   auto path = std::string { };
   auto name = std::string_view { phys_addrof(request->name).get() };

   if (request->fileType == MCPFileType::CafeOS) {
      if (std::find(decaf::config()->system.lle_modules.begin(),
                    decaf::config()->system.lle_modules.end(),
                    name) == decaf::config()->system.lle_modules.end()) {
         auto library = cafe::hle::getLibrary(name);
         if (library) {
            auto &rpl = library->getGeneratedRpl();
            return static_cast<MCPError>(rpl.size());
         }
      }
   }

   switch (request->fileType) {
   case MCPFileType::ProcessCode:
      path = fmt::format("/vol/code/{}", name);
      break;
   case MCPFileType::CafeOS:
      path = fmt::format("/vol/storage_mlc01/sys/title/00050010/1000400A/code/{}", name);
      break;
   case MCPFileType::SharedDataCode:
      path = fmt::format("/vol/storage_mlc01/sys/title/0005001B/10042400/code/{}", name);
      break;
   case MCPFileType::SharedDataContent:
      path = fmt::format("/vol/storage_mlc01/sys/title/0005001B/10042400/content/{}", name);
      break;
   default:
      return static_cast<MCPError>(Error::InvalidArg);
   }

   StackObject<FSAStat> stat;
   auto error = FSAGetInfoByQuery(getFsaHandle(),
                                  path,
                                  FSAQueryInfoType::Stat,
                                  stat);
   if (error < FSAStatus::OK) {
      return static_cast<MCPError>(error);
   }

   return static_cast<MCPError>(stat->size);
}

MCPError
mcpGetSysProdSettings(phys_ptr<MCPResponseGetSysProdSettings> response)
{
   std::memcpy(phys_addrof(response->settings).get(),
               getSysProdConfig().get(),
               sizeof(MCPSysProdSettings));
   return MCPError::OK;
}

MCPError
mcpGetTitleId(phys_ptr<kernel::ResourceRequest> resourceRequest,
              phys_ptr<MCPResponseGetTitleId> response)
{
   response->titleId = resourceRequest->requestData.titleId;
   return MCPError::OK;
}

MCPError
mcpLoadFile(phys_ptr<const MCPRequestLoadFile> request,
            phys_ptr<void> outputBuffer,
            uint32_t outputBufferLength)
{
   auto path = std::string { };
   auto name = std::string_view { phys_addrof(request->name).get() };

   if (request->fileType == MCPFileType::CafeOS) {
      if (std::find(decaf::config()->system.lle_modules.begin(),
                    decaf::config()->system.lle_modules.end(),
                    name) == decaf::config()->system.lle_modules.end()) {
         auto library = cafe::hle::getLibrary(name);
         if (library) {
            auto &rpl = library->getGeneratedRpl();
            auto bytesRead = std::min<uint32_t>(static_cast<uint32_t>(rpl.size() - request->pos),
                                                outputBufferLength);
            std::memcpy(outputBuffer.get(),
                        rpl.data() + request->pos,
                        bytesRead);
            return static_cast<MCPError>(bytesRead);
         }
      }
   }

   switch (request->fileType) {
   case MCPFileType::ProcessCode:
      path = fmt::format("/vol/code/{}", name);
      break;
   case MCPFileType::CafeOS:
      path = fmt::format("/vol/system_slc/title/00050010/1000400A/code/{}", name);
      break;
   case MCPFileType::SharedDataCode:
      path = fmt::format("/vol/storage_mlc01/sys/title/0005001B/10042400/code/{}", name);
      break;
   case MCPFileType::SharedDataContent:
      path = fmt::format("/vol/storage_mlc01/sys/title/0005001B/10042400/content/{}", name);
      break;
   default:
      return static_cast<MCPError>(Error::InvalidArg);
   }

   auto fsaHandle = getFsaHandle();
   if (path != sCurrentLoadFilePath) {
      auto fileHandle = FSAHandle { };

      // Open a new file
      auto error = FSAOpenFile(fsaHandle, path, "r", &fileHandle);
      if (error < 0) {
         return static_cast<MCPError>(error);
      }

      StackObject<FSAStat> stat;
      error = FSAStatFile(fsaHandle, fileHandle, stat);
      if (error < 0) {
         FSACloseFile(fsaHandle, fileHandle);
         return static_cast<MCPError>(error);
      }

      sCurrentLoadFileSize = stat->size;
      sCurrentLoadFileHandle = fileHandle;
      sCurrentLoadFilePath = path;
   }

   auto error = FSAReadFileWithPos(fsaHandle,
                                   outputBuffer,
                                   1,
                                   outputBufferLength,
                                   request->pos,
                                   sCurrentLoadFileHandle,
                                   FSAReadFlag::None);
   if (error < 0 || request->pos + error >= sCurrentLoadFileSize) {
      FSACloseFile(fsaHandle, sCurrentLoadFileHandle);
      sCurrentLoadFileSize = 0u;
      sCurrentLoadFileHandle = static_cast<FSAHandle>(Error::Invalid);
      sCurrentLoadFilePath.clear();
   }

   return static_cast<MCPError>(error);
}

static bool
checkExistenceUsingOpenDir(std::string_view path)
{
   auto fsaHandle = getFsaHandle();
   auto dirHandle = FSADirHandle { -1 };
   auto result = FSAOpenDir(fsaHandle, path, &dirHandle);
   if (result == FSAStatus::OK) {
      FSACloseDir(fsaHandle, dirHandle);
      return true;
   }

   if (result == FSAStatus::NotDir) {
      return true;
   }

   return false;
}

static bool
checkExistence(std::string_view path)
{
   StackObject<FSAStat> stat;
   auto result = FSAGetStat(getFsaHandle(), path, stat);
   if (result == FSAStatus::OK) {
      return true;
   }

   if (result == FSAStatus::PermissionError) {
      return checkExistenceUsingOpenDir(path);
   }

   return false;
}

MCPError
mcpPrepareTitle52(phys_ptr<const MCPRequestPrepareTitle> request,
                  phys_ptr<MCPResponsePrepareTitle> response)
{
   auto titleInfoBuffer = getPrepareTitleInfoBuffer();
   auto titleId = request->titleId;
   if (titleId == DefaultTitleId) {
      StackObject<MCPTitleAppXml> appXml;
      if (auto error = readTitleAppXml(appXml); error < MCPError::OK) {
         titleInfoBuffer = getPrepareTitleInfoBuffer();
         std::memset(titleInfoBuffer.get(), 0x0, sizeof(MCPPPrepareTitleInfo));
         titleInfoBuffer->permissions[0].group = static_cast<uint32_t>(ResourcePermissionGroup::All);
         titleInfoBuffer->permissions[0].mask = 0xFFFFFFFFFFFFFFFFull;
         return error;
      }

      titleInfoBuffer->titleId = appXml->title_id;
      titleInfoBuffer->groupId = appXml->group_id;

      titleId = appXml->title_id;
   }

   // TODO: When we have title switching we will need to read the title id and
   // mount the correct title to /vol - until then libdecaf already mounted it.
   auto error = readTitleCosXml(titleInfoBuffer);
   if (error < MCPError::OK) {
      // If there is no cos.xml then let's grant full permissions
      std::memset(titleInfoBuffer.get(), 0x0, sizeof(MCPPPrepareTitleInfo));
      titleInfoBuffer->permissions[0].group = static_cast<uint32_t>(ResourcePermissionGroup::All);
      titleInfoBuffer->permissions[0].mask = 0xFFFFFFFFFFFFFFFFull;
   } else {
      // If there is cos.xml but it doesn't have any permissions then grant full permissions
      bool havePermissions = false;
      for (auto i = 0u; i <= 18; ++i) {
         if (titleInfoBuffer->permissions[i].group != 0 ||
             titleInfoBuffer->permissions[i].mask != 0) {
            havePermissions = true;
            break;
         }
      }
      if (!havePermissions) {
         titleInfoBuffer->permissions[0].group = static_cast<uint32_t>(ResourcePermissionGroup::All);
         titleInfoBuffer->permissions[0].mask = 0xFFFFFFFFFFFFFFFFull;
      }
   }

   // Try mount updates for the title
   auto titleIdLo = titleId & 0xFFFFFFFF;
   auto titleUpdatePath = fmt::format("/vol/storage_mlc01/usr/title/0005000e/{:08x}", titleIdLo);
   if (checkExistence(titleUpdatePath)) {
      gLog->info("Title update found at {}", titleUpdatePath);

      auto processInfo = FSAProcessInfo { };
      processInfo.groupId = titleInfoBuffer->groupId;
      processInfo.titleId = titleInfoBuffer->titleId;
      processInfo.processId = ios::ProcessId::COSKERNEL; // TODO: Use correct process id.

      auto fsaHandle = getFsaHandle();
      auto mountResult =
         FSAMountWithProcess(fsaHandle, titleUpdatePath + "/code",
                             "/vol/code", FSAMountPriority::TitleUpdate,
                             &processInfo, nullptr, 0);
      if (mountResult) {
         gLog->warn("Error mounting update path {}/code to /vol/code",
                    titleUpdatePath);
      } else {
         gLog->info("Mounted update {}/code to /vol/code", titleUpdatePath);
      }

      mountResult =
         FSAMountWithProcess(fsaHandle, titleUpdatePath + "/content",
                             "/vol/content", FSAMountPriority::TitleUpdate,
                             &processInfo, nullptr, 0);
      if (mountResult) {
         gLog->warn("Error mounting update path {}/content to /vol/content",
                    titleUpdatePath + "/content");
      } else {
         gLog->info("Mounted update {}/content to /vol/content", titleUpdatePath);
      }

      mountResult =
         FSAMountWithProcess(fsaHandle, titleUpdatePath + "/meta",
                             "/vol/meta", FSAMountPriority::TitleUpdate,
                             &processInfo, nullptr, 0);
      if (mountResult) {
         gLog->warn("Error mounting update path {}/meta to /vol/meta",
                    titleUpdatePath + "/meta");
      } else {
         gLog->info("Mounted update {}/meta to /vol/meta", titleUpdatePath);
      }
   } else {
      gLog->info("No title update found at {}", titleUpdatePath);
   }

   // Return result
   std::memcpy(phys_addrof(response->titleInfo).get(),
               titleInfoBuffer.get(),
               sizeof(MCPPPrepareTitleInfo));
   std::memset(phys_addrof(response->titleInfo.permissions).get(),
               0, sizeof(response->titleInfo.permissions));
   return MCPError::OK;
}

MCPError
mcpSwitchTitle(phys_ptr<const MCPRequestSwitchTitle> request)
{
   auto titleInfoBuffer = getPrepareTitleInfoBuffer();
   auto processId = static_cast<ProcessId>(ProcessId::COSKERNEL + request->cafeProcessId);
   auto sdCardPermissions = vfs::NoPermissions;

   // Apply title permissions
   for (auto &permission : titleInfoBuffer->permissions) {
      if (!permission.group) {
         break;
      }

      IOS_SetClientCapabilities(processId,
                                permission.group,
                                phys_addrof(permission.mask));

      if (permission.group == ResourcePermissionGroup::FS ||
          permission.group == ResourcePermissionGroup::All) {
         if (permission.mask & FSResourcePermissions::SdCardRead) {
            sdCardPermissions = sdCardPermissions | vfs::OtherRead;
         }

         if (permission.mask & FSResourcePermissions::SdCardWrite) {
            sdCardPermissions = sdCardPermissions | vfs::OtherWrite;
         }
      }
   }

   IOS_SetProcessTitle(processId,
                       titleInfoBuffer->titleId,
                       titleInfoBuffer->groupId);

   // Mount sdcard if title has permissions
   if (sdCardPermissions != vfs::NoPermissions) {
      auto filesystem = ios::getFileSystem();
      filesystem->mountDevice({}, "/dev/sdcard01",
                              std::make_shared<vfs::HostDevice>(decaf::config()->system.sdcard_path));
      filesystem->setPermissions({}, "/dev/sdcard01", sdCardPermissions);
   }

   std::memset(titleInfoBuffer.get(),
               0xFF,
               sizeof(MCPPPrepareTitleInfo));
   return MCPError::OK;
}

MCPError
mcpUpdateCheckContext(phys_ptr<MCPResponseUpdateCheckContext> response)
{
   // TODO: Implement mcpUpdateCheckContext
   std::memset(response.get(), 0, sizeof(MCPResponseUpdateCheckContext));
   return MCPError::OK;
}

MCPError
mcpUpdateCheckResume(phys_ptr<MCPResponseUpdateCheckResume> response)
{
   // TODO: Implement mcpUpdateCheckResume
   std::memset(response.get(), 0, sizeof(MCPResponseUpdateCheckResume));
   return MCPError::OK;
}

MCPError
mcpUpdateGetProgress(phys_ptr<MCPResponseUpdateProgress> response)
{
   // TODO: Implement mcpUpdateGetProgress
   std::memset(response.get(), 0, sizeof(MCPResponseUpdateProgress));
   return MCPError::OK;
}

} // namespace ios::mcp::internal
