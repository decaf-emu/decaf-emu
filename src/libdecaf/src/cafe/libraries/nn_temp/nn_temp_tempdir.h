#pragma once
#include "nn_temp_enum.h"
#include "cafe/libraries/coreinit/coreinit_fs.h"

#include <libcpu/be2_struct.h>
#include <string>

namespace cafe::nn_temp
{

#pragma pack(push, 1)

using TEMPDirId = uint64_t;
constexpr auto DirPathMaxLength = 0x20u;
constexpr auto GlobalPathMaxLength = 0x40u;

struct TEMPDeviceInfo
{
   be2_val<uint64_t> dirId;
   be2_array<char, GlobalPathMaxLength> targetPath;
};
CHECK_OFFSET(TEMPDeviceInfo, 0x00, dirId);
CHECK_OFFSET(TEMPDeviceInfo, 0x08, targetPath);
CHECK_SIZE(TEMPDeviceInfo, 0x48);

#pragma pack(pop)

TEMPStatus
TEMPInit();

void
TEMPShutdown();

TEMPStatus
TEMPCreateAndInitTempDir(uint32_t maxSize,
                         TEMPDevicePreference pref,
                         virt_ptr<TEMPDirId> outDirId);

TEMPStatus
TEMPGetDirPath(TEMPDirId dirId,
               virt_ptr<char> pathBuffer,
               uint32_t pathBufferSize);

TEMPStatus
TEMPGetDirGlobalPath(TEMPDirId dirId,
                     virt_ptr<char> pathBuffer,
                     uint32_t pathBufferSize);

TEMPStatus
TEMPShutdownTempDir(TEMPDirId id);

} // namespace cafe::nn_temp
