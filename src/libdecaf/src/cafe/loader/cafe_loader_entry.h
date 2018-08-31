#pragma once
#include "cafe_loader_minfileinfo.h"

#include "cafe/kernel/cafe_kernel_context.h"
#include "cafe/kernel/cafe_kernel_processid.h"
#include <libcpu/be2_struct.h>

namespace cafe::loader
{

struct LOADED_RPL;

enum class LOADER_Code : int32_t
{
   Invalid                    = -1,
   Prep                       = 1,
   Setup                      = 2,
   Purge                      = 3,
   Link                       = 4,
   Query                      = 5,
   Tag                        = 6,
   UserGainControl            = 7,
   Done                       = 8,
   GetLoaderHeapStatistics    = 9,
};

struct LOADER_EntryDispatch
{
   be2_val<LOADER_Code> code;
   be2_val<LOADER_Handle> handle;
   be2_virt_ptr<LOADER_MinFileInfo> minFileInfo;
   be2_virt_ptr<LOADER_LinkInfo> linkInfo;
   be2_val<uint32_t> linkInfoSize;
   UNKNOWN(0x20 - 0x14);
};
CHECK_OFFSET(LOADER_EntryDispatch, 0x00, code);
CHECK_OFFSET(LOADER_EntryDispatch, 0x04, handle);
CHECK_OFFSET(LOADER_EntryDispatch, 0x08, minFileInfo);
CHECK_OFFSET(LOADER_EntryDispatch, 0x0C, linkInfo);
CHECK_OFFSET(LOADER_EntryDispatch, 0x10, linkInfoSize);
CHECK_SIZE(LOADER_EntryDispatch, 0x20);

struct LOADER_EntryParams
{
   be2_virt_ptr<kernel::Context> procContext;
   be2_val<kernel::UniqueProcessId> procId;
   be2_val<int32_t> procConfig;
   be2_virt_ptr<kernel::Context> context;
   be2_val<BOOL> interruptsAllowed;
   be2_val<uint32_t> unk0x14;
   be2_struct<LOADER_EntryDispatch> dispatch;
};
CHECK_OFFSET(LOADER_EntryParams, 0x00, procContext);
CHECK_OFFSET(LOADER_EntryParams, 0x04, procId);
CHECK_OFFSET(LOADER_EntryParams, 0x08, procConfig);
CHECK_OFFSET(LOADER_EntryParams, 0x0C, context);
CHECK_OFFSET(LOADER_EntryParams, 0x10, interruptsAllowed);
CHECK_OFFSET(LOADER_EntryParams, 0x14, unk0x14);
CHECK_OFFSET(LOADER_EntryParams, 0x18, dispatch);
CHECK_SIZE(LOADER_EntryParams, 0x38);

int32_t
LoaderStart(BOOL isEntryCall,
            virt_ptr<LOADER_EntryParams> entryParams);

void
lockLoader();

void
unlockLoader();

virt_ptr<LOADED_RPL>
getLoadedRpx();

virt_ptr<LOADED_RPL>
getLoadedRplLinkedList();

namespace internal
{

uint32_t
getProcTitleLoc();

kernel::ProcessFlags
getProcFlags();

} // namespace internal

} // namespace cafe::loader
