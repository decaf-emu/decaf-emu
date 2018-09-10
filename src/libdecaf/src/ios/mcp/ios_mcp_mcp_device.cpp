#include "ios_mcp_config.h"
#include "ios_mcp_enum.h"
#include "ios_mcp_mcp_device.h"
#include "ios_mcp_mcp_types.h"
#include "ios_mcp_mcp_request.h"
#include "ios_mcp_mcp_response.h"
#include "ios_mcp_mcp_thread.h"

#include "cafe/libraries/cafe_hle.h"
#include "ios/ios_stackobject.h"
#include "ios/fs/ios_fs_fsa_ipc.h"
#include "ios/kernel/ios_kernel_process.h"

namespace ios::mcp::internal
{

using namespace ios::fs;

static std::string
sCurrentLoadFilePath = { };

static FSAHandle
sCurrentLoadFileHandle = { };

static size_t
sCurrentLoadFileSize = 0u;

MCPError
mcpGetSysProdSettings(phys_ptr<MCPResponseGetSysProdSettings> response)
{
   std::memcpy(phys_addrof(response->settings).getRawPointer(),
               getSysProdConfig().getRawPointer(),
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
   auto name = std::string_view { phys_addrof(request->name).getRawPointer() };

   if (request->fileType == MCPFileType::CafeOS) {
      auto library = cafe::hle::getLibrary(name);
      if (library) {
         auto &rpl = library->getGeneratedRpl();
         auto bytesRead = std::min<uint32_t>(rpl.size() - request->pos,
                                             outputBufferLength);
         std::memcpy(outputBuffer.getRawPointer(),
                     rpl.data() + request->pos,
                     bytesRead);
         return static_cast<MCPError>(bytesRead);
      }
   }

   switch (request->fileType) {
   case MCPFileType::ProcessCode:
      path = fmt::format("/vol/code/{}", name);
      break;
   case MCPFileType::CafeOS:
      path = fmt::format("/vol/storage_mlc01/sys/title/00050010/1000400A/code/{}", name);
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

} // namespace ios::mcp::internal
