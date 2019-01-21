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

namespace ios::mcp::internal
{

using namespace ios::fs;
using namespace ios::kernel;

static std::string sCurrentLoadFilePath = { };
static FSAHandle sCurrentLoadFileHandle = { };
static size_t sCurrentLoadFileSize = 0u;

MCPError
mcpGetFileLength(phys_ptr<MCPRequestGetFileLength> request)
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
   auto fsaHandle = getFsaHandle();
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
mcpLoadFile(phys_ptr<MCPRequestLoadFile> request,
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

   auto bytesRead = uint32_t { 0 };
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

MCPError
mcpPrepareTitle52(phys_ptr<MCPRequestPrepareTitle> request,
                  phys_ptr<MCPResponsePrepareTitle> response)
{
   auto titleInfoBuffer = getPrepareTitleInfoBuffer();
   auto titleId = request->titleId;
   auto groupId = GroupId { 0 };
   if (titleId == DefaultTitleId) {
      StackObject<MCPTitleAppXml> appXml;
      if (auto error = readTitleAppXml(appXml); error < MCPError::OK) {
         auto titleInfoBuffer = getPrepareTitleInfoBuffer();
         std::memset(titleInfoBuffer.get(), 0x0, sizeof(MCPPPrepareTitleInfo));
         return error;
      }

      titleInfoBuffer->titleId = appXml->title_id;
      titleInfoBuffer->groupId = appXml->group_id;
   }

   // TODO: When we have title switching we will need to read the title id and
   // load the correct title, until then our title is already mounted on /vol
   auto error = readTitleCosXml(titleInfoBuffer);
   if (error < MCPError::OK) {
      // If there is no cos.xml then let's grant full permissions
      std::memset(titleInfoBuffer.get(), 0x0, sizeof(MCPPPrepareTitleInfo));
      titleInfoBuffer->permissions[0].group = static_cast<uint32_t>(ResourcePermissionGroup::All);
      titleInfoBuffer->permissions[0].mask = 0xFFFFFFFFFFFFFFFFull;
   }

   std::memcpy(phys_addrof(response->titleInfo).get(),
               titleInfoBuffer.get(),
               sizeof(MCPPPrepareTitleInfo));
   std::memset(phys_addrof(response->titleInfo.permissions).get(),
               0, sizeof(response->titleInfo.permissions));
   return MCPError::OK;
}

MCPError
mcpSwitchTitle(phys_ptr<MCPRequestSwitchTitle> request)
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

} // namespace ios::mcp::internal
