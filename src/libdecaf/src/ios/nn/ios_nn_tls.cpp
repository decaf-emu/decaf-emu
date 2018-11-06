#include "ios_nn_tls.h"

#include "ios/kernel/ios_kernel_process.h"
#include "ios/kernel/ios_kernel_thread.h"

#include <array>
#include <libcpu/be2_struct.h>

using namespace ios::kernel;

namespace nn
{

struct NnThreadLocalStorage
{
   static constexpr auto Magic = 0x94826575u;

   UNKNOWN(0x4);
   be2_phys_ptr<void> tlsData;
   be2_val<uint32_t> magic;
   UNKNOWN(0x24 - 0x0C);
};
CHECK_OFFSET(NnThreadLocalStorage, 0x04, tlsData);
CHECK_OFFSET(NnThreadLocalStorage, 0x08, magic);
CHECK_SIZE(NnThreadLocalStorage, 0x24);

struct TlsTable
{
   TlsEntry *entries = nullptr;
   uint32_t dataSize = 0u;
};

static std::array<TlsTable, NumIosProcess> sProcessTlsTables;

static TlsTable *
getTlsTable()
{
   auto error = IOS_GetCurrentProcessId();
   if (error < ::ios::Error::OK) {
      return nullptr;
   }

   return &sProcessTlsTables[static_cast<size_t>(error)];
}


/**
 * Allocate a TLS entry for the current process.
 */
void
tlsAllocateEntry(TlsEntry &entry,
                 TlsEntryEventFn eventCallback,
                 bool supportsCopy)
{
   auto table = getTlsTable();

   // Initialise entry
   entry.eventFn = eventCallback;
   entry.supportsCopy = supportsCopy;

   // Assign data position
   auto size = eventCallback(TlsEntryEvent::GetSize, nullptr, nullptr);
   entry.dataOffset = table->dataSize;
   table->dataSize += size;

   // Insert into table
   entry.next = table->entries;
   table->entries = &entry;
}


/**
 * Get the data size of all TLS entries in the current process.
 *
 * This should be the size of the data passed to tlsInitialiseData.
 */
uint32_t
tlsGetDataSize()
{
   return getTlsTable()->dataSize;
}


/**
 * Initialise a TLS data block for the current process.
 */
void
tlsInitialiseData(phys_ptr<void> data,
                  phys_ptr<void> copySrc)
{
   auto table = getTlsTable();
   if (copySrc) {
      decaf_abort("TODO: Implement tlsInitialiseData for copies");
   }

   auto dataAddr = phys_cast<phys_addr>(data);
   std::memset(data.get(), 0, table->dataSize);

   for (auto entry = table->entries; entry; entry = entry->next) {
      entry->eventFn(TlsEntryEvent::Create,
                     phys_cast<void *>(dataAddr + entry->dataOffset),
                     nullptr);
   }
}


/**
 * Clean up a TLS data block for the current process.
 */
void
tlsDestroyData(phys_ptr<void> data)
{
   auto table = getTlsTable();
   auto dataAddr = phys_cast<phys_addr>(data);
   for (auto entry = table->entries; entry; entry = entry->next) {
      entry->eventFn(TlsEntryEvent::Destroy,
                     phys_cast<void *>(dataAddr + entry->dataOffset),
                     nullptr);
   }
}


/**
 * Initialise the current thread's local storage.
 */
void
tlsInitialiseThread(phys_ptr<void> data)
{
   auto tls = phys_cast<NnThreadLocalStorage *>(IOS_GetCurrentThreadLocalStorage());
   tls->magic = NnThreadLocalStorage::Magic;
   tls->tlsData = data;
}


/**
 * Get the data pointer for the current thread's TLS entry.
 */
phys_ptr<void>
tlsGetEntry(TlsEntry &entry)
{
   auto tls = phys_cast<NnThreadLocalStorage *>(IOS_GetCurrentThreadLocalStorage());
   if (!tls || !tls->tlsData) {
      return nullptr;
   }

   return phys_cast<void *>(phys_cast<phys_addr>(tls->tlsData) + entry.dataOffset);
}

} // namespace nn
