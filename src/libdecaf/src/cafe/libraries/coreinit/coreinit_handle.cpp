#include "coreinit.h"
#include "coreinit_handle.h"
#include "coreinit_spinlock.h"
#include "coreinit_systemheap.h"
#include "coreinit_time.h"
#include "cafe/cafe_ppc_interface_invoke.h"
#include <cstdlib>
#include <libcpu/cpu.h>

namespace cafe::coreinit
{

static internal::SubTableAllocFn sAllocSubTable;
static internal::SubTableFreeFn sFreeSubTable;

namespace internal
{

static OSHandleError
Handle_InitTable(virt_ptr<HandleTable> table,
                 SubTableAllocFn allocSubTableFn,
                 SubTableFreeFn freeSubTableFn)
{
   if (!table) {
      return OSHandleError::InvalidArgument;
   }

   std::memset(table.getRawPointer(), 0, sizeof(HandleTable));
   table->allocSubTableFn = allocSubTableFn;
   table->freeSubTableFn = freeSubTableFn;
   table->handleEntropy = 0xCAFEu;
   table->subTables[0] = virt_addrof(table->firstSubTable);
   table->subTableFreeEntries[0] = HandleSubTable::NumEntries;
   return OSHandleError::OK;
}

static OSHandleError
Handle_Alloc(virt_ptr<HandleTable> table,
             virt_ptr<void> userData1,
             virt_ptr<void> userData2,
             virt_ptr<OSHandle> outHandle)
{
   if (!table || !outHandle) {
      return OSHandleError::InvalidArgument;
   }

   auto firstFreeSubTableIdx = -1;
   auto subTableIdx = -1;

   // Find a sub table with a free entry
   for (auto i = 0; i < HandleTable::NumSubTables; ++i) {
      if (table->subTables[i]) {
         if (table->subTableFreeEntries[i]) {
            subTableIdx = i;
            break;
         }
      } else if (firstFreeSubTableIdx) {
         firstFreeSubTableIdx = i;
      }
   }

   if (subTableIdx == -1) {
      if (firstFreeSubTableIdx == -1) {
         // Table completely full
         return OSHandleError::TableFull;
      }

      // Allocate a new empty sub table
      subTableIdx = firstFreeSubTableIdx;
      auto subTable = cafe::invoke(cpu::this_core::state(),
                                   table->allocSubTableFn);

      std::memset(subTable.getRawPointer(), 0, sizeof(HandleSubTable));
      table->subTables[subTableIdx] = subTable;
      table->subTableFreeEntries[subTableIdx] = HandleSubTable::NumEntries;
   }

   auto subTable = table->subTables[subTableIdx];
   auto entryIndex = static_cast<uint32_t>(rand()) % HandleSubTable::NumEntries;
   auto firstEntryIndex = entryIndex;

   while (subTable->entries[entryIndex].handle) {
      ++entryIndex;

      if (entryIndex == HandleSubTable::NumEntries) {
         entryIndex = 0;
      }

      if (entryIndex == firstEntryIndex) {
         // Somehow we have not found a free entry, this is a major fuckup and
         // should actually be impossible...
         return OSHandleError::InternalError;
      }
   }

   auto handleEntropy = (table->handleEntropy + OSGetTick() % 0x20041) ^ 0x1A5A;
   if (!handleEntropy) {
      handleEntropy = 1;
   }

   table->handleEntropy = handleEntropy;

   // Thanks Hex-Rays!
   auto handleIndex = entryIndex | (subTableIdx << 9);
   auto v21 = ((handleIndex + 1) & 0x1FFFF) | ((handleEntropy << 17) & 0x7FE0000);
   auto v22 =
      (((v21 & 0x55555555) + ((v21 & 0xAAAAAAAA) >> 1)) & 0x33333333)
      + ((((v21 & 0x55555555) + ((v21 & 0xAAAAAAAA) >> 1)) & 0xCCCCCCCC) >> 2);
   auto handle =
      (0xF8000000 *
         ((((v22 & 0xF0F0F0F) + ((v22 & 0xF0F0F0F0) >> 4)) & 0xFF00FF)
            + ((((v22 & 0xF0F0F0F) + ((v22 & 0xF0F0F0F0) >> 4)) & 0xFF00FF00) >> 8)
            + (((((v22 & 0xF0F0F0F) + ((v22 & 0xF0F0F0F0) >> 4)) & 0xFF00FF)
            + ((((v22 & 0xF0F0F0F) + ((v22 & 0xF0F0F0F0) >> 4)) & 0xFF00FF00) >> 8)) >> 16))
         & 0xF8000000)
      | (v21 & 0x7FFFFFF);

   --table->subTableFreeEntries[subTableIdx];

   auto &entry = subTable->entries[entryIndex];
   entry.handle = static_cast<OSHandle>(handle);
   entry.userData1 = userData1;
   entry.userData2 = userData2;
   entry.refCount = 1u;

   *outHandle = handle;
   return OSHandleError::OK;
}

static OSHandleError
Handle_TranslateAndAddRef(virt_ptr<HandleTable> table,
                          OSHandle handle,
                          virt_ptr<virt_ptr<void>> outUserData1,
                          virt_ptr<virt_ptr<void>> outUserData2)
{
   if (!table || !handle) {
      return OSHandleError::InvalidArgument;
   }

   auto handleIndex = (handle - 1) & 0x1FFFF;
   auto subTableIndex = handleIndex >> 9;
   auto entryIndex = handleIndex & 0x1FF;

   if (subTableIndex > HandleTable::NumSubTables ||
       entryIndex > HandleSubTable::NumEntries) {
      return OSHandleError::InvalidHandle;
   }

   auto subTable = table->subTables[subTableIndex];
   if (!subTable) {
      return OSHandleError::InvalidHandle;
   }

   auto &entry = subTable->entries[entryIndex];
   if (entry.handle != handle) {
      return OSHandleError::InvalidHandle;
   }

   entry.refCount++;

   if (outUserData1) {
      *outUserData1 = entry.userData1;
   }

   if (outUserData2) {
      *outUserData2 = entry.userData2;
   }

   return OSHandleError::OK;
}

static OSHandleError
Handle_Release(virt_ptr<HandleTable> table,
               OSHandle handle,
               virt_ptr<uint32_t> outRefCount)
{
   if (!table || !handle) {
      return OSHandleError::InvalidArgument;
   }

   auto handleIndex = (handle - 1) & 0x1FFFF;
   auto subTableIndex = handleIndex >> 9;
   auto entryIndex = handleIndex & 0x1FF;

   if (subTableIndex > HandleTable::NumSubTables ||
       entryIndex > HandleSubTable::NumEntries) {
      return OSHandleError::InvalidHandle;
   }

   auto subTable = table->subTables[subTableIndex];
   if (!subTable) {
      return OSHandleError::InvalidHandle;
   }

   auto &entry = subTable->entries[entryIndex];
   if (entry.handle != handle) {
      return OSHandleError::InvalidHandle;
   }

   entry.refCount--;

   if (outRefCount) {
      *outRefCount = entry.refCount;
   }

   if (!entry.refCount) {
      std::memset(virt_addrof(entry).getRawPointer(), 0, sizeof(HandleEntry));
      table->subTableFreeEntries[subTableIndex]++;

      // Free the sub table if it is completely empty and was a dynamically
      // allocated sub table (index > 0).
      if (subTableIndex > 0 &&
          table->subTableFreeEntries[subTableIndex] == HandleSubTable::NumEntries &&
          table->freeSubTableFn) {
         cafe::invoke(cpu::this_core::state(),
                      table->freeSubTableFn,
                      subTable);

         table->subTableFreeEntries[subTableIndex] = 0u;
         table->subTables[subTableIndex] = nullptr;
      }
   }

   return OSHandleError::OK;
}

static virt_ptr<HandleSubTable>
allocSubTable()
{
   return virt_cast<HandleSubTable *>(
      OSAllocFromSystem(sizeof(HandleSubTable), 4)
   );
}

static void
freeSubTable(virt_ptr<HandleSubTable> table)
{
   OSFreeToSystem(table);
}

} // namespace internal


/**
 * Initialise a handle table.
 *
 * \param table
 * The handle table to initialise.
 *
 * \return
 * OSHandleError::OK on success, a OSHandleError error code otherwise.
 */
OSHandleError
OSHandle_InitTable(virt_ptr<OSHandleTable> table)
{
   if (!table) {
      return OSHandleError::InvalidArgument;
   }

   std::memset(table.getRawPointer(), 0, sizeof(OSHandleTable));

   auto error = internal::Handle_InitTable(virt_addrof(table->handleTable),
                                           sAllocSubTable,
                                           sFreeSubTable);
   if (error == OSHandleError::OK) {
      OSInitSpinLock(virt_addrof(table->lock));
   }

   return error;
}


/**
 * Allocate a new handle from the handle table.
 *
 * \param table
 * The handle table to allocate the handle from.
 *
 * \param userData1
 * User data to set in the handle entry, can be read from
 * OSHandle_TranslateAndAddRef.
 *
 * \param userData2
 * User data to set in the handle entry, can be read from
 * OSHandle_TranslateAndAddRef.
 *
 * \param[out] outHandle
 * Output parameter, set to the handle value for the newly acquired handle.
 * Must not be nullptr.
 *
 * \return
 * OSHandleError::OK on success, a OSHandleError error code otherwise.
 */
OSHandleError
OSHandle_Alloc(virt_ptr<OSHandleTable> table,
               virt_ptr<void> userData1,
               virt_ptr<void> userData2,
               virt_ptr<OSHandle> outHandle)
{
   if (!table || !outHandle) {
      return OSHandleError::InvalidArgument;
   }

   OSUninterruptibleSpinLock_Acquire(virt_addrof(table->lock));
   auto error = internal::Handle_Alloc(virt_addrof(table->handleTable),
                                       userData1,
                                       userData2,
                                       outHandle);
   OSUninterruptibleSpinLock_Release(virt_addrof(table->lock));
   return error;
}


/**
 * Increase the reference count of a handle.
 *
 * \param table
 * The handle table to acquire the handle from.
 *
 * \param handle
 * The handle to acquire.
 *
 * \return
 * OSHandleError::OK on success, a OSHandleError error code otherwise.
 */
OSHandleError
OSHandle_AddRef(virt_ptr<OSHandleTable> table,
                OSHandle handle)
{
   return OSHandle_TranslateAndAddRef(table, handle, nullptr, nullptr);
}


/**
 * Increase the reference count of a handle and retrieve it's user data values.
 *
 * \param table
 * The handle table to acquire the handle from.
 *
 * \param handle
 * The handle to acquire.
 *
 * \param[out] outUserData1
 * Optional output parameter, set to the userData1 value for the handle which
 * was passed into OSHandle_Alloc.
 *
 * \param[out] outUserData2
 * Optional output parameter, set to the userData2 value for the handle which
 * was passed into OSHandle_Alloc.
 *
 * \return
 * OSHandleError::OK on success, a OSHandleError error code otherwise.
 */
OSHandleError
OSHandle_TranslateAndAddRef(virt_ptr<OSHandleTable> table,
                            OSHandle handle,
                            virt_ptr<virt_ptr<void>> outUserData1,
                            virt_ptr<virt_ptr<void>> outUserData2)
{
   if (!table || !handle) {
      return OSHandleError::InvalidArgument;
   }

   OSUninterruptibleSpinLock_Acquire(virt_addrof(table->lock));
   auto error =
      internal::Handle_TranslateAndAddRef(virt_addrof(table->handleTable),
                                          handle,
                                          outUserData1,
                                          outUserData2);
   OSUninterruptibleSpinLock_Release(virt_addrof(table->lock));
   return error;
}


/**
 * Reduce the reference count of a handle and free it if the count reaches 0.
 *
 * \param table
 * The handle table to release the handle from.
 *
 * \param handle
 * The handle to release.
 *
 * \param[out] outRefCount
 * Optional output parameter, set to the value of the new ref count.
 *
 * \return
 * OSHandleError::OK on success, a OSHandleError error code otherwise.
 */
OSHandleError
OSHandle_Release(virt_ptr<OSHandleTable> table,
                 OSHandle handle,
                 virt_ptr<uint32_t> outRefCount)
{
   if (!table || !handle) {
      return OSHandleError::InvalidArgument;
   }

   OSUninterruptibleSpinLock_Acquire(virt_addrof(table->lock));
   auto error =
      internal::Handle_Release(virt_addrof(table->handleTable),
                               handle,
                               outRefCount);
   OSUninterruptibleSpinLock_Release(virt_addrof(table->lock));
   return error;
}

void
Library::registerHandleSymbols()
{
   RegisterFunctionExport(OSHandle_AddRef);
   RegisterFunctionExport(OSHandle_Alloc);
   RegisterFunctionExport(OSHandle_InitTable);
   RegisterFunctionExport(OSHandle_Release);
   RegisterFunctionExport(OSHandle_TranslateAndAddRef);

   RegisterFunctionInternal(internal::allocSubTable, sAllocSubTable);
   RegisterFunctionInternal(internal::freeSubTable, sFreeSubTable);
}

} // namespace cafe::coreinit
