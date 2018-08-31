#pragma once
#include "coreinit_enum.h"
#include "coreinit_spinlock.h"
#include <libcpu/be2_struct.h>

namespace cafe::coreinit
{

using OSHandle = uint32_t;

namespace internal
{

struct HandleEntry;
struct HandleSubTable;
struct HandleTable;

using SubTableAllocFn = virt_func_ptr<virt_ptr<HandleSubTable>()>;
using SubTableFreeFn = virt_func_ptr<void(virt_ptr<HandleSubTable>)>;

struct HandleEntry
{
   be2_val<OSHandle> handle;
   be2_virt_ptr<void> userData1;
   be2_virt_ptr<void> userData2;
   be2_val<uint32_t> refCount;
};

struct HandleSubTable
{
   static constexpr auto NumEntries = 0x200u;

   be2_array<HandleEntry, NumEntries> entries;
};

struct HandleTable
{
   static constexpr auto NumSubTables = 0x100u;

   be2_val<SubTableAllocFn> allocSubTableFn;
   be2_val<SubTableFreeFn> freeSubTableFn;
   be2_val<uint32_t> handleEntropy;
   be2_array<uint32_t, NumSubTables> subTableFreeEntries;
   be2_array<virt_ptr<HandleSubTable>, NumSubTables> subTables;
   be2_struct<HandleSubTable> firstSubTable;
};
CHECK_OFFSET(HandleTable, 0x00, allocSubTableFn);
CHECK_OFFSET(HandleTable, 0x04, freeSubTableFn);
CHECK_OFFSET(HandleTable, 0x08, handleEntropy);
CHECK_OFFSET(HandleTable, 0x0C, subTableFreeEntries);
CHECK_OFFSET(HandleTable, 0x40C, subTables);
CHECK_OFFSET(HandleTable, 0x80C, firstSubTable);
CHECK_SIZE(HandleTable, 0x280C);

} // namespace internal

struct OSHandleTable
{
   be2_struct<internal::HandleTable> handleTable;
   PADDING(4);
   be2_struct<OSSpinLock> lock;
};
CHECK_SIZE(OSHandleTable, 0xA08 * 4);

OSHandleError
OSHandle_InitTable(virt_ptr<OSHandleTable> table);

OSHandleError
OSHandle_Alloc(virt_ptr<OSHandleTable> table,
               virt_ptr<void> userData1,
               virt_ptr<void> userData2,
               virt_ptr<OSHandle> outHandle);

OSHandleError
OSHandle_AddRef(virt_ptr<OSHandleTable> table,
                OSHandle handle);

OSHandleError
OSHandle_TranslateAndAddRef(virt_ptr<OSHandleTable> table,
                            OSHandle handle,
                            virt_ptr<virt_ptr<void>> outUserData1,
                            virt_ptr<virt_ptr<void>> outUserData2);

OSHandleError
OSHandle_Release(virt_ptr<OSHandleTable> table,
                 OSHandle handle,
                 virt_ptr<uint32_t> outRefCount);

} // namespace cafe::coreinit
