#pragma once
#include <cstdint>
#include <common/bitfield.h>

namespace cafe::kernel
{

enum class UniqueProcessId : int32_t
{
   Invalid                 = -1,
   Kernel                  = 0,
   Root                    = 1,
   HomeMenu                = 2,
   TV                      = 3,
   EManual                 = 4,
   OverlayMenu             = 5,
   ErrorDisplay            = 6,
   MiniMiiverse            = 7,
   InternetBrowser         = 8,
   Miiverse                = 9,
   EShop                   = 10,
   FLV                     = 11,
   DownloadManager         = 12,
   Game                    = 15,
};

constexpr auto NumRamPartitions = 8;

enum class RamPartitionId : int32_t
{
   Invalid                 = -1,
   Kernel                  = 0,
   Root                    = 1,
   Loader                  = 2,
   OverlayApp              = 4,
   OverlayMenu             = 5,
   ErrorDisplay            = 6,
   MainApplication         = 7,
};

enum class KernelProcessId : int32_t
{
   Invalid                 = -1,
   Kernel                  = 0,
   Loader                  = 2,
};

enum class DebugLevel : uint32_t
{
   Error                   = 0,
   Warn                    = 1,
   Info                    = 2,
   Notice                  = 3,
   Verbose                 = 7,
};

BITFIELD(ProcessFlags, uint32_t)
   BITFIELD_ENTRY(0, 1, bool, isFirstProcess);
   BITFIELD_ENTRY(1, 1, bool, disableSharedLibraries);
   BITFIELD_ENTRY(9, 3, DebugLevel, debugLevel);
   BITFIELD_ENTRY(12, 1, bool, unkBit12);
   BITFIELD_ENTRY(27, 1, bool, isDebugMode);
   BITFIELD_ENTRY(29, 1, bool, isColdBoot);
BITFIELD_END

using TitleId = uint64_t;

RamPartitionId
getRamPartitionIdFromUniqueProcessId(UniqueProcessId id);

} // namespace cafe::kernel
