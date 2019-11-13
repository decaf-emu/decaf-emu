#include "cafe_kernel_userdrivers.h"
#include "cafe_kernel_lock.h"
#include "cafe_kernel_heap.h"
#include "cafe_kernel_process.h"

#include "cafe/cafe_stackobject.h"

#include <cctype>
#include <common/strutils.h>

namespace cafe::kernel
{

struct UserDriver
{
   be2_array<char, 64> name;
   be2_val<UniqueProcessId> ownerUpid;
   be2_val<uint32_t> unk0x44;
   be2_virt_ptr<UserDriver> next;
};

struct StaticUserDriversData
{
   be2_virt_ptr<UserDriver> registeredDrivers;
};

static virt_ptr<StaticUserDriversData>
sUserDriversData = nullptr;

static internal::SpinLock
sDriverLock { 0 };

int32_t
registerUserDriver(virt_ptr<const char> name,
                   uint32_t nameLen,
                   virt_ptr<UniqueProcessId> currentUpid,
                   virt_ptr<UniqueProcessId> ownerUpid)
{
   // Get a lowercase copy of name
   StackArray<char, 64> nameCopy;
   string_copy(nameCopy.get(), nameCopy.size(), name.get(), std::max<uint32_t>(nameLen, 63u));
   nameCopy[63] = 0;

   for (auto i = 0u; i < nameCopy.size() && nameCopy[i]; ++i) {
      nameCopy[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(nameCopy[i])));
   }

   auto upid = internal::getCurrentUniqueProcessId();
   if (currentUpid) {
      *currentUpid = upid;
   }

   internal::spinLockAcquire(sDriverLock, cpu::this_core::id() + 1);

   // Check if this driver has already been registered
   for (auto itr = sUserDriversData->registeredDrivers; itr; itr = itr->next) {
      if (std::strncmp(virt_addrof(itr->name).get(), nameCopy.get(), 64) == 0) {
         if (ownerUpid) {
            *ownerUpid = itr->ownerUpid;
            internal::spinLockRelease(sDriverLock, cpu::this_core::id() + 1);
            return 0;
         }
      }
   }

   // Add driver to list
   auto driver = virt_cast<UserDriver *>(internal::allocFromWorkArea(sizeof(UserDriver)));
   std::memcpy(virt_addrof(driver->name).get(), nameCopy.get(), driver->name.size());
   driver->ownerUpid = upid;
   driver->next = sUserDriversData->registeredDrivers;
   sUserDriversData->registeredDrivers = driver;
   internal::spinLockRelease(sDriverLock, cpu::this_core::id() + 1);

   if (ownerUpid) {
      *ownerUpid = upid;
   }

   return 0;
}

int32_t
deregisterUserDriver(virt_ptr<const char> name,
                     uint32_t nameLen)
{
   // Get a lowercase copy of name
   StackArray<char, 64> nameCopy;
   string_copy(nameCopy.get(), nameCopy.size(), name.get(), std::max<uint32_t>(nameLen, 63u));
   nameCopy[63] = 0;

   for (auto i = 0u; i < nameCopy.size() && nameCopy[i]; ++i) {
      nameCopy[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(nameCopy[i])));
   }

   // Remove driver with same name & upid from list
   auto upid = internal::getCurrentUniqueProcessId();
   auto prev = virt_ptr<UserDriver> { nullptr };
   internal::spinLockAcquire(sDriverLock, cpu::this_core::id() + 1);
   for (auto itr = sUserDriversData->registeredDrivers; itr; itr = itr->next) {
      if (std::strncmp(virt_addrof(itr->name).get(), nameCopy.get(), 64) == 0) {
         if (itr->ownerUpid == upid) {
            if (prev) {
               prev->next = itr->next;
            } else {
               sUserDriversData->registeredDrivers = itr->next;
            }

            internal::freeToWorkArea(itr);
         }

         break;
      }

      prev = itr;
   }
   internal::spinLockRelease(sDriverLock, cpu::this_core::id() + 1);
   return 0;
}

namespace internal
{

void
initialiseStaticUserDriversData()
{
   sUserDriversData = allocStaticData<StaticUserDriversData>();
}

/**
 * These drivers are normally registered by root.rpx as part of the boot
 * process. We must register them here so when they are subsequently reigstered
 * by an application they will see the kernel driver already has been
 * associated with the root process and behave appropriately.
 *
 * Essentially this is a hack because we do not emulate the root.rpx process.
 * It's times like this where I regret not writing an LLE emulator ... :)
 */
void
registerRootUserDrivers()
{
   auto registerRootDriver = [](const char *name) {
      auto driver = virt_cast<UserDriver *>(internal::allocFromWorkArea(sizeof(UserDriver)));
      driver->name = name;
      driver->ownerUpid = UniqueProcessId::Root;
      driver->next = sUserDriversData->registeredDrivers;
      sUserDriversData->registeredDrivers = driver;
   };

   // Registered via call from root.rpx to PADInit
   registerRootDriver("pad");

   // Registered via call from root.rpx to WPADInit
   registerRootDriver("wpad");

   // Registered via call from root.rpx to VPADInit
   registerRootDriver("vpad");
}

} // namespace internal

} // namespace cafe::kernel
