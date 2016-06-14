#pragma once
#include "common/be_val.h"
#include "common/structsize.h"
#include "virtual_ptr.h"

namespace coreinit
{

/**
 * \defgroup coreinit_memlist Memory List
 * \ingroup coreinit
 *
 * A linked list used for memory heaps.
 * @{
 */

#pragma pack(push, 1)

struct MemoryLink
{
   be_ptr<void> prev;
   be_ptr<void> next;
};

CHECK_OFFSET(MemoryLink, 0x0, prev);
CHECK_OFFSET(MemoryLink, 0x4, next);
CHECK_SIZE(MemoryLink, 0x8);

struct MemoryList
{
   be_ptr<void> head;
   be_ptr<void> tail;
   be_val<uint16_t> count;
   be_val<uint16_t> offsetToMemoryLink;
};
CHECK_OFFSET(MemoryList, 0x0, head);
CHECK_OFFSET(MemoryList, 0x4, tail);
CHECK_OFFSET(MemoryList, 0x8, count);
CHECK_OFFSET(MemoryList, 0xa, offsetToMemoryLink);
CHECK_SIZE(MemoryList, 0xc);

#pragma pack(pop)

void
MEMInitList(MemoryList *list, uint16_t offsetToMemoryLink);

void
MEMAppendListObject(MemoryList *list, void *object);

void
MEMPrependListObject(MemoryList *list, void *object);

void
MEMInsertListObject(MemoryList *list, void *before, void *object);

void
MEMRemoveListObject(MemoryList *list, void *object);

void *
MEMGetNextListObject(MemoryList *list, void *object);

void *
MEMGetPrevListObject(MemoryList *list, void *object);

void *
MEMGetNthListObject(MemoryList *list, uint16_t n);

/** @} */

} // namespace coreinit
