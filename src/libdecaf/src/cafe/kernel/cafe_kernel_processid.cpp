#include "cafe_kernel_processid.h"

namespace cafe::kernel
{

RamPartitionId
getRamPartitionIdFromUniqueProcessId(UniqueProcessId id)
{
   switch (id) {
   case UniqueProcessId::Kernel:
      return RamPartitionId::Kernel;
   case UniqueProcessId::Root:
      return RamPartitionId::Root;
   case UniqueProcessId::HomeMenu:
      return RamPartitionId::MainApplication;
   case UniqueProcessId::TV:
      return RamPartitionId::OverlayApp;
   case UniqueProcessId::EManual:
      return RamPartitionId::OverlayApp;
   case UniqueProcessId::OverlayMenu:
      return RamPartitionId::OverlayMenu;
   case UniqueProcessId::ErrorDisplay:
      return RamPartitionId::ErrorDisplay;
   case UniqueProcessId::MiniMiiverse:
      return RamPartitionId::OverlayApp;
   case UniqueProcessId::InternetBrowser:
      return RamPartitionId::OverlayApp;
   case UniqueProcessId::Miiverse:
      return RamPartitionId::OverlayApp;
   case UniqueProcessId::EShop:
      return RamPartitionId::OverlayApp;
   case UniqueProcessId::FLV:
      return RamPartitionId::OverlayApp;
   case UniqueProcessId::DownloadManager:
      return RamPartitionId::OverlayApp;
   case UniqueProcessId::Game:
      return RamPartitionId::MainApplication;
   default:
      decaf_abort(fmt::format("Unknown UniqueProcessId {}", static_cast<uint32_t>(id)));
   }
}

} // namespace cafe::kernel
