#include "ios_mcp.h"
#include "ios_mcp_config.h"
#include "ios_mcp_enum.h"
#include "ios_mcp_mcp_device.h"
#include "ios_mcp_mcp_thread.h"
#include "ios_mcp_mcp_response.h"
#include "ios_mcp_pm_thread.h"

#include "ios/auxil/ios_auxil_config.h"

#include "ios/fs/ios_fs_fsa_ipc.h"

#include "ios/kernel/ios_kernel_debug.h"
#include "ios/kernel/ios_kernel_messagequeue.h"
#include "ios/kernel/ios_kernel_process.h"
#include "ios/kernel/ios_kernel_resourcemanager.h"
#include "ios/kernel/ios_kernel_thread.h"

#include "ios/ios_stackobject.h"

#include <common/log.h>

namespace ios::mcp::internal
{

constexpr auto McpThreadStackSize = 0x8000u;
constexpr auto McpThreadPriority = 123u;
constexpr auto MaxNumMessages = 200u;
constexpr auto MaxNumMcpHandles = 256u;

using MCPHandle = int32_t;

using namespace fs;
using namespace kernel;

struct StaticData
{
   be2_val<uint32_t> systemMode;
   be2_val<FSAHandle> fsaHandle;
   be2_val<ThreadId> threadId;
   be2_array<uint8_t, McpThreadStackSize> threadStack;
   be2_array<Message, MaxNumMessages> messageBuffer;
   be2_array<uint8_t, MaxNumMcpHandles / 8> handleOpenBitset;
};

static phys_ptr<StaticData>
sData;

static MCPError
mcpIoctl(phys_ptr<ResourceRequest> request)
{
   auto error = MCPError::OK;
   auto &ioctl = request->requestData.args.ioctl;

   switch (static_cast<MCPCommand>(request->requestData.args.ioctl.request)) {
   case MCPCommand::GetTitleId:
      if (ioctl.outputLength == sizeof(MCPResponseGetTitleId)) {
         error = mcpGetTitleId(request,
                               phys_cast<MCPResponseGetTitleId>(ioctl.outputBuffer));
      } else {
         error = MCPError::InvalidParam;
      }
      break;
   default:
      error = MCPError::Opcode;
   }

   return error;
}

static MCPError
mcpIoctlv(phys_ptr<ResourceRequest> request)
{
   auto error = MCPError::OK;
   auto &ioctlv = request->requestData.args.ioctlv;

   switch (static_cast<MCPCommand>(ioctlv.request)) {
   case MCPCommand::GetSysProdSettings:
      if (ioctlv.numVecIn == 0 &&
          ioctlv.numVecOut == 1 &&
          ioctlv.vecs[0].paddr &&
          ioctlv.vecs[0].len == sizeof(MCPResponseGetSysProdSettings)) {
         error = mcpGetSysProdSettings(phys_ptr<MCPResponseGetSysProdSettings>(ioctlv.vecs[0].paddr));
      } else {
         error = MCPError::InvalidParam;
      }
      break;
   default:
      error = MCPError::Opcode;
   }

   return error;
}

static bool
isOpenHandle(MCPHandle handle)
{
   if (handle < 0) {
      return false;
   }

   auto handleIndex = handle >> 16;
   if (handleIndex >= MaxNumMcpHandles) {
      return false;
   }

   auto byteIndex = handleIndex / 8;
   auto bitIndex = handleIndex % 8;
   return sData->handleOpenBitset[byteIndex] & (1 << bitIndex);
}

static Error
mcpOpen(ClientCapabilityMask caps)
{
   auto handleIndex = -1;
   auto byteIndex = 0;
   auto bitIndex = 0;

   for (auto i = 0u; i < MaxNumMcpHandles; ++i) {
      byteIndex = i / 8;
      bitIndex = i % 8;

      if ((sData->handleOpenBitset[byteIndex] & (1 << bitIndex)) == 0) {
         handleIndex = i;
         break;
      }
   }

   if (handleIndex < 0) {
      return Error::Max;
   }

   // Set the open bit
   sData->handleOpenBitset[byteIndex] |= (1 << bitIndex);

   auto handle = (handleIndex << 16) | static_cast<int32_t>(caps & 0xFFFF);
   return static_cast<Error>(handle);
}

static Error
mcpClose(MCPHandle handle)
{
   auto handleIndex = handle >> 16;
   if (handleIndex >= MaxNumMcpHandles) {
      return Error::InvalidHandle;
   }

   // Clear the open bit
   auto byteIndex = handleIndex / 8;
   auto bitIndex = handleIndex % 8;
   sData->handleOpenBitset[byteIndex] &= ~(1 << bitIndex);

   return Error::OK;
}

static MCPError
initialiseClientCaps()
{
   StackObject<uint64_t> mask;

   struct
   {
      ProcessId pid;
      FeatureId fid;
      uint64_t mask;
   } caps[] = {
      { ProcessId::CRYPTO,        1,                       0xFF },
      { ProcessId::USB,           1,                        0xF },
      { ProcessId::USB,           0xC,       0xFFFFFFFFFFFFFFFF },
      { ProcessId::USB,           9,         0xFFFFFFFFFFFFFFFF },
      { ProcessId::USB,           0xB,                0x3300300 },
      { ProcessId::FS,            0xB,           0x400000000F00 },
      { ProcessId::FS,            0xD,                        1 },
      { ProcessId::FS,            0xC,                        1 },
      { ProcessId::PAD,           0xB,                 0x101000 },
      { ProcessId::PAD,           1,                        0xF },
      { ProcessId::PAD,           0xD,                     0x11 },
      { ProcessId::PAD,           2,         0xFFFFFFFFFFFFFFFF },
      { ProcessId::PAD,           0x18,      0xFFFFFFFFFFFFFFFF },
      { ProcessId::NET,           1,                        0xF },
      { ProcessId::NET,           8,                          1 },
      { ProcessId::NET,           0xB,               0x101B1001 },
      { ProcessId::NET,           3,                          3 },
      { ProcessId::NET,           0xE,                     0x10 },
      { ProcessId::NET,           0x10,                   0x800 },
      { ProcessId::NET,           0x11,                       8 },
      { ProcessId::NET,           2,         0xFFFFFFFFFFFFFFFF },
      { ProcessId::NET,           0x12,                     0xF },
      { ProcessId::NET,           0x14,                     0xF },
      { ProcessId::NET,           0xC,                        1 },
      { ProcessId::NET,           0x1A,      0xFFFFFFFFFFFFFFFF },
      { ProcessId::NET,           0xD,                     0x11 },
      { ProcessId::ACP,           0xB,       0xFFFFFFFFF33F3091 },
      { ProcessId::ACP,           0xD,                     0x11 },
      { ProcessId::ACP,           1,                        0xF },
      { ProcessId::ACP,           0x12,                     0xF },
      { ProcessId::ACP,           0xF,                        1 },
      { ProcessId::ACP,           0x10,                    0xFF },
      { ProcessId::ACP,           2,         0xFFFFFFFFFFFFFFFF },
      { ProcessId::ACP,           0xE,                   0x27CB },
      { ProcessId::ACP,           0x11,                     0xA },
      { ProcessId::ACP,           0x14,                     0xF },
      { ProcessId::ACP,           9,                          1 },
      { ProcessId::NSEC,          0xB,                 0x303300 },
      { ProcessId::NSEC,          0xD,                        1 },
      { ProcessId::NSEC,          2,         0xFFFFFFFFFFFFFFFF },
      { ProcessId::FPD,           0xB,                0x3303000 },
      { ProcessId::FPD,           0xD,                     0x11 },
      { ProcessId::FPD,           0x12,                     0xF },
      { ProcessId::FPD,           0xF,                        3 },
      { ProcessId::FPD,           2,         0xFFFFFFFFFFFFFFFF },
      { ProcessId::FPD,           0x14,                     0xF },
      { ProcessId::FPD,           0x16,      0xFFFFFFFFFFFFFFFF },
      { ProcessId::NIM,           0xB,            0x200303B3000 },
      { ProcessId::NIM,           1,                        0xF },
      { ProcessId::NIM,           0xD,                     0x15 },
      { ProcessId::NIM,           0x12,                    0x13 },
      { ProcessId::NIM,           0xF,                        3 },
      { ProcessId::NIM,           0x11,                       3 },
      { ProcessId::NIM,           2,         0xFFFFFFFFFFFFFFFF },
      { ProcessId::NIM,           0x13,      0xFFFFFFFFFFFFFFFF },
      { ProcessId::NIM,           0x14,                     0xF },
      { ProcessId::NIM,           0x16,      0xFFFFFFFFFFFFFFFF },
      { ProcessId::AUXIL,         0xB,                0x3300300 },
      { ProcessId::TEST,          1,                        0xF },
      { ProcessId::TEST,          3,                          3 },
      { ProcessId::TEST,          0xB,       0xFFFFFFFFF03FFF00 },
      { ProcessId::TEST,          0xD,                        1 },
      { ProcessId::TEST,          0x16,                       1 },
      { ProcessId::COSKERNEL,     1,                     0xFF00 },
      { ProcessId::COSKERNEL,     3,                          6 },
      { ProcessId::COSKERNEL,     0xD,                        1 },
      { ProcessId::COSROOT,       1,                      0xF00 },
      { ProcessId::COSROOT,       3,                          6 },
      { ProcessId::COSROOT,       0xD,                     0x11 },
   };

   for (auto &cap : caps) {
      *mask = cap.mask;
      IOS_SetClientCapabilities(cap.pid, cap.fid, mask);
   }

   return MCPError::OK;
}

static FSAStatus
initialiseDirectories()
{
   static const struct
   {
      const char *path;
      uint32_t changeOwnerArg0;
      uint32_t changeOwnerArg1;
      uint32_t changeOwnerArg3;
      ProcessId changeOwnerArg2;
      uint32_t mode;
      uint32_t sysModePcfs;
      uint32_t sysModeNand;
      uint32_t sysModeSdcard;
      int32_t makeQuotaSizeHi;
      uint32_t makeQuotaSizeLo;
      uint32_t flushBit;
   } sDirs[] = {
      { "/vol/system_slc/logs",             0,          0,     0, ProcessId::MCP,       0, 0x164000, 0x1C4000,  0x40000, -1, 0xFFFFFFFC, 2 },
      { "/vol/system_slc/rights",           0,          0,     0, ProcessId::MCP,       0, 0x164000,  0x44000,  0x40000, -1, 0xFFFFFFFC, 2 },
      { "/vol/system_slc/rights/ticket",    0,          0,     0, ProcessId::MCP,       0, 0x164000,  0x44000,  0x40000, -1, 0xFFFFFFFC, 2 },
      { "/vol/system_slc/title",            0,          0,     0, ProcessId::MCP,       0, 0x164000,  0x44000,  0x40000, -1, 0xFFFFFFFC, 2 },
      { "/vol/system_slc/import",           0,          0,     0, ProcessId::MCP,       0, 0x164000,  0x44000,  0x40000, -1, 0xFFFFFFFC, 2 },
      { "/vol/system_slc/security",         0,          0,     0, ProcessId::MCP,       0, 0x164000,  0x44000,  0x40000, -1, 0xFFFFFFFC, 2 },
      { "/vol/system_slc/config",           0,          0,     0, ProcessId::MCP,       0, 0x164000,  0x44000,  0x40000, -1, 0xFFFFFFFC, 2 },
      { "/vol/system_slc/proc",             0,          0,     0, ProcessId::MCP,       0, 0x164000,  0x44000,  0x40000, -1, 0xFFFFFFFC, 2 },
      { "/vol/system_slc/proc/acp",         0, 0x100000F6,     0, ProcessId::ACP,   0x600, 0x164000,  0x44000,  0x40000, -1, 0xFFFFFFFC, 2 },
      { "/vol/system_slc/proc/prefs",       0, 0x100000F5,     0, ProcessId::AUXIL, 0x600, 0x164000,  0x44000,  0x40000, -1, 0xFFFFFFFC, 2 },
      { "/vol/system_slc/proc/usb",         0, 0x100000FA,     0, ProcessId::USB,   0x600, 0x1E4000, 0x1C4000,  0x40000, -1, 0xFFFFFFFC, 0 },
      { "/vol/system_slc/tmp",              0,          0,     0, ProcessId::MCP,       0, 0x164000,  0x44000,  0x40000, -1, 0xFFFFFFFC, 2 },
      { "/vol/storage_mlc01/sys",           0,          0,     0, ProcessId::MCP,       0, 0x164000,  0x44000,  0x40000,  0, 0xC0000000, 1 },
      { "/vol/storage_mlc01/usr",           0,          0,     0, ProcessId::MCP,       0, 0x164000,  0x44000,  0x40000, -1, 0xFFFFFFFC, 1 },
      { "/vol/storage_mlc01/sys/title",     0,          0,     0, ProcessId::MCP,       0, 0x164000,  0x44000,  0x40000, -1, 0xFFFFFFFC, 1 },
      { "/vol/storage_mlc01/sys/config",    0,          0,     0, ProcessId::MCP,       0, 0x164000, 0x144000,  0x40000, -1, 0xFFFFFFFC, 1 },
      { "/vol/storage_mlc01/sys/import",    0,          0,     0, ProcessId::MCP,       0, 0x164000,  0x44000,  0x40000, -1, 0xFFFFFFFC, 1 },
      { "/vol/storage_mlc01/sys/update",    0, 0x100000F3,     0, ProcessId::NIM,   0x600, 0x164000,  0x44000,  0x40000, -1, 0xFFFFFFFC, 1 },
      { "/vol/storage_mlc01/usr/tmp",       0,          0,     0, ProcessId::MCP,   0x666, 0x164000,  0x44000,  0x40000, -1, 0xFFFFFFFC, 1 },
      { "/vol/storage_mlc01/usr/title",     0,          0,     0, ProcessId::MCP,       0, 0x164000,  0x44000,  0x40000, -1, 0xFFFFFFFC, 1 },
      { "/vol/storage_mlc01/usr/import",    0,          0,     0, ProcessId::MCP,       0, 0x164000, 0x144000,  0x40000, -1, 0xFFFFFFFC, 1 },
      { "/vol/storage_mlc01/usr/save",      0, 0x100000F6,     0, ProcessId::ACP,   0x600, 0x164000,  0x44000,  0x40000, -1, 0xFFFFFFFC, 1 },
      { "/vol/storage_mlc01/usr/boss",      0, 0x100000F6,     0, ProcessId::ACP,   0x600, 0x164000,  0x44000,  0x40000, -1, 0xFFFFFFFC, 1 },
      { "/vol/storage_mlc01/usr/nsec",      0, 0x100000F4,     0, ProcessId::NSEC,  0x600, 0x164000, 0x144000,  0x40000,  0,   0xA00000, 1 },
      { "/vol/storage_mlc01/usr/packages",  0, 0x100000F3,     0, ProcessId::NIM,   0x600, 0x164000,  0x44000,  0x40000, -1, 0xFFFFFFFC, 1 },
      { "/vol/system_ram/cache",            0,          0,     0, ProcessId::MCP,       0, 0x1A4000, 0x184000, 0x140000,  0,  0x6400000, 4 },
      { "/vol/system_ram/es",               0,          0,     0, ProcessId::MCP,       0, 0x1E4000, 0x1C4000, 0x140000,  0,   0x300000, 4 },
      { "/vol/system_ram/config",           0,          0,     0, ProcessId::MCP,       0, 0x1A4000, 0x184000, 0x140000, -1, 0xFFFFFFFC, 4 },
      { "/vol/system_ram/proc",             0,          0,     0, ProcessId::MCP,       0, 0x1A4000, 0x184000, 0x140000, -1, 0xFFFFFFFC, 4 },
      { "/vol/system_ram/proc/cache",       0,          0,     0, ProcessId::MCP,   0x600, 0x1A0000, 0x180000, 0x140000,  0,   0xA00000, 4 },
      { "/vol/system_ram/proc/fpd",         0, 0x100000F7, 0x400, ProcessId::FPD,   0x600, 0x1A4000, 0x184000, 0x140000,  0,    0x40000, 4 },
      { "/vol/system_ram/proc/prefs",       0, 0x100000F5,     0, ProcessId::AUXIL, 0x600, 0x1A4000, 0x184000, 0x140000,  0,   0x100000, 4 },
      { "/vol/system_ram/proc/usb",         0, 0x100000FA,     0, ProcessId::USB,   0x600, 0x1A4000, 0x184000, 0x140000,  0,    0x40000, 4 },
      { "/vol/system_hfio/config",          0,          0,     0, ProcessId::MCP,       0, 0x1A0000,        0,        0, -1, 0xFFFFFFFC, 0 },
      { "/vol/system_hfio/proc",            0,          0,     0, ProcessId::MCP,       0, 0x1A0000,        0,        0, -1, 0xFFFFFFFC, 0 },
      { "/vol/system_hfio/proc/acp",        0, 0x100000F6,     0, ProcessId::ACP,   0x600, 0x1A0000,        0,        0, -1, 0xFFFFFFFC, 0 },
      { "/vol/system_hfio/proc/usb",        0, 0x100000FA,     0, ProcessId::USB,   0x600, 0x1A0000,        0,        0, -1, 0xFFFFFFFC, 0 },
      { "/vol/system_hfio/proc/prefs",      0, 0x100000F5,     0, ProcessId::AUXIL, 0x600, 0x1A0000,        0,        0, -1, 0xFFFFFFFC, 0 },
      { "/vol/system_hfio/logs",            0,          0,     0, ProcessId::MCP,       0, 0x120000,        0,        0, -1, 0xFFFFFFFC, 0 },
   };

   StackObject<FSAStat> stat;
   auto flushBits = 0u;
   auto systemFileSys = getSystemFileSys();
   auto systemModeFlags = getSystemModeFlags();

   for (auto &dir : sDirs) {
      auto dirSystemMode = dir.sysModeNand;

      if (systemFileSys == SystemFileSys::Pcfs) {
         dirSystemMode = dir.sysModePcfs;
      } else if (systemFileSys == SystemFileSys::SdCard) {
         dirSystemMode = dir.sysModeSdcard;
      }

      if (!(systemModeFlags & dirSystemMode)) {
         continue;
      }

      auto error = FSAGetInfoByQuery(sData->fsaHandle, dir.path, FSAQueryInfoType::Stat, stat);
      if (error == FSAStatus::NotFound) {
         if (dir.makeQuotaSizeHi == -1) {
            error = FSAMakeDir(sData->fsaHandle, dir.path, dir.mode);
         } else {
            auto quota = (static_cast<uint64_t>(dir.makeQuotaSizeHi) << 32) | dir.makeQuotaSizeLo;
            error = FSAMakeQuota(sData->fsaHandle, dir.path, dir.mode, quota);
         }

         if (error == FSAStatus::OK) {
            // TODO: FSAChangeOwner
         }
      } else {
         // TODO: FSAChangeMode
      }

      if (error < FSAStatus::OK) {
         return error;
      }

      flushBits |= dir.flushBit;
   }

   if (flushBits & 1) {
      // TODO: FSAFlushVolume "/vol/storage_mlc01"
   }

   if (flushBits & 2) {
      // TODO: FSAFlushVolume "/vol/system_slc"
   }

   if (flushBits & 4) {
      // TODO: FSAFlushVolume "/vol/system_ram"
   }

   return FSAStatus::OK;
}

static Error
cleanTmpDirectories()
{
   // TODO: Clean dirs in /vol/system_slc/tmp
   // TODO: Clean dirs in /vol/storage_mlc01/usr/tmp
   return Error::OK;
}

static MCPError
mcpResume()
{
   auto iosError = FSAOpen();
   if (iosError < Error::OK) {
      gLog->error("Failed to open FSA handle");
      return MCPError::Invalid;
   }

   sData->fsaHandle = static_cast<FSAHandle>(iosError);

   // Mount the system devices
   auto fsaStatus = FSAMount(sData->fsaHandle, "/dev/slc01", "/vol/system_slc", 0, nullptr, 0);
   if (fsaStatus < Error::OK) {
      gLog->error("Failed to mount /dev/slc01 to /vol/system_slc");
      return MCPError::Invalid;
   }

   fsaStatus = FSAMount(sData->fsaHandle, "/dev/mlc01", "/vol/storage_mlc01", 0, nullptr, 0);
   if (fsaStatus < Error::OK) {
      gLog->error("Failed to mount /dev/mlc01 to /vol/storage_mlc01");
      return MCPError::Invalid;
   }

   fsaStatus = FSAMount(sData->fsaHandle, "/dev/ramdisk01", "/vol/system_ram", 0, nullptr, 0);
   if (fsaStatus < Error::OK) {
      gLog->error("Failed to mount /dev/ramdisk01 to /vol/system_ram");
      return MCPError::Invalid;
   }

   auto securityLevel = IOS_GetSecurityLevel();
   if (getSystemFileSys() == SystemFileSys::Pcfs && securityLevel != SecurityLevel::Normal) {
      fsaStatus = FSAMount(sData->fsaHandle, "/dev/hfio01", "/vol/system_hfio", 0, nullptr, 0);
      if (fsaStatus < Error::OK) {
         gLog->error("Failed to mount /dev/hfio01 to /vol/system_hfio");
         return MCPError::Invalid;
      }
   }

   // Cleanup tmp directories
   auto error = cleanTmpDirectories();
   if (error < Error::OK) {
      return MCPError::Invalid;
   }

   // Initialise the default directories
   initialiseDirectories();

   // Mount /vol/system
   if (getSystemFileSys() == SystemFileSys::Pcfs) {
      fsaStatus = FSAMount(sData->fsaHandle, "/vol/system_hfio", "/vol/system", 0, nullptr, 0);
      if (fsaStatus < Error::OK) {
         gLog->error("Failed to mount /vol/system_hfio to /vol/system");
         return MCPError::Invalid;
      }
   } else {
      fsaStatus = FSAMount(sData->fsaHandle, "/vol/system_slc", "/vol/system", 0, nullptr, 0);
      if (fsaStatus < Error::OK) {
         gLog->error("Failed to mount /vol/system_slc to /vol/system");
         return MCPError::Invalid;
      }
   }

   // Mount /vol/sys/proc
   fsaStatus = FSAMount(sData->fsaHandle, "/vol/system/proc", "/vol/sys/proc", 0, nullptr, 0);
   if (fsaStatus < Error::OK) {
      gLog->error("Failed to mount /vol/system/proc to /vol/sys/proc");
      return MCPError::Invalid;
   }

   fsaStatus = FSAMount(sData->fsaHandle, "/vol/system_slc/proc", "/vol/sys/proc_slc", 0, nullptr, 0);
   if (fsaStatus < Error::OK) {
      gLog->error("Failed to mount /vol/system_slc/proc to /vol/sys/proc_slc");
      return MCPError::Invalid;
   }

   fsaStatus = FSAMount(sData->fsaHandle, "/vol/system_ram/proc", "/vol/sys/proc_ram", 0, nullptr, 0);
   if (fsaStatus < Error::OK) {
      gLog->error("Failed to mount /vol/system_ram/proc to /vol/sys/proc_ram");
      return MCPError::Invalid;
   }

   // Open FSA handle for config
   iosError = auxil::openFsaHandle();
   if (iosError < Error::OK) {
      gLog->error("Failed to open config FSA handle");
      return MCPError::Invalid;
   }

   // Load configs
   auto mcpError = loadRtcConfig();
   if (mcpError < MCPError::OK) {
      gLog->error("Failed to initialise rtc config");
      return mcpError;
   }

   mcpError = loadSystemConfig();
   if (mcpError < MCPError::OK) {
      gLog->error("Failed to initialise system config");
      return mcpError;
   }

   mcpError = loadSysProdConfig();
   if (mcpError < MCPError::OK) {
      gLog->error("Failed to initialise sys_prod config");
      return mcpError;
   }

   // Init client caps
   mcpError = initialiseClientCaps();
   if (mcpError < MCPError::OK) {
      gLog->error("Failed to initialise client capabilities");
      return mcpError;
   }

   if (securityLevel == SecurityLevel::Debug || securityLevel == SecurityLevel::Test) {
      fsaStatus = FSAMount(sData->fsaHandle, "/dev/hfio01", "/vol/storage_hfiomlc01", 0, nullptr, 0);
      if (fsaStatus < Error::OK) {
         gLog->error("Failed to mount /dev/hfio01 to /vol/storage_hfiomlc01");
         return MCPError::Invalid;
      }
   }

   if (getSystemFileSys() == SystemFileSys::Nand) {
      getSystemConfig()->ramdisk.cache_user_code = 1u;
   }

   return MCPError::OK;
}

static MCPError
mcpSuspend()
{
   // TODO: mcp suspend
   return MCPError::OK;
}

static Error
mcpThreadEntry(phys_ptr<void> /*context*/)
{
   StackObject<Message> message;

   auto error = IOS_CreateMessageQueue(phys_addrof(sData->messageBuffer),
                                       sData->messageBuffer.size());
   if (error < Error::OK) {
      return error;
   }

   auto messageQueueId = static_cast<MessageQueueId>(error);
   error = registerResourceManager("/dev/mcp", messageQueueId);
   if (error < Error::OK) {
      return error;
   }

   error = IOS_AssociateResourceManager("/dev/mcp", ResourcePermissionGroup::MCP);
   if (error < Error::OK) {
      return error;
   }

   while (true) {
      error = IOS_ReceiveMessage(messageQueueId,
                                 message,
                                 MessageFlags::None);
      if (error < Error::OK) {
         break;
      }

      auto request = parseMessage<ResourceRequest>(message);
      switch (request->requestData.command) {
      case Command::Open:
         if (!(request->requestData.args.open.caps & 1)) {
            IOS_ResourceReply(request, Error::Access);
         } else {
            error = mcpOpen(request->requestData.args.open.caps);
            IOS_ResourceReply(request, error);
         }
         break;
      case Command::Close:
         IOS_ResourceReply(request, mcpClose(request->requestData.handle));
         break;
      case Command::Ioctl:
         if (!isOpenHandle(request->requestData.handle)) {
            IOS_ResourceReply(request, Error::InvalidHandle);
         } else {
            IOS_ResourceReply(request, static_cast<Error>(mcpIoctl(request)));
         }
         break;
      case Command::Ioctlv:
         if (!isOpenHandle(request->requestData.handle)) {
            IOS_ResourceReply(request, Error::InvalidHandle);
         } else {
            IOS_ResourceReply(request, static_cast<Error>(mcpIoctlv(request)));
         }
         break;
      case Command::Suspend:
         IOS_ResourceReply(request, static_cast<Error>(mcpSuspend()));
         break;
      case Command::Resume:
         IOS_ResourceReply(request, static_cast<Error>(mcpResume()));
         break;
      default:
         IOS_ResourceReply(request, Error::InvalidArg);
      }
   }

   return error;
}

Error
startMcpThread()
{
   auto error = IOS_CreateThread(&mcpThreadEntry,
                                 nullptr,
                                 phys_addrof(sData->threadStack) + sData->threadStack.size(),
                                 sData->threadStack.size(),
                                 McpThreadPriority,
                                 ThreadFlags::Detached);
   if (error < Error::OK) {
      return error;
   }

   return IOS_StartThread(error);
}

void
initialiseStaticMcpThreadData()
{
   sData = allocProcessStatic<StaticData>();
}

} // namespace ios::mcp::internal
