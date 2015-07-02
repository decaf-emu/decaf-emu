#pragma once
#include "systemtypes.h"

#pragma pack(push, 1)

struct MemoryList
{
   be_ptr<void> head;
   be_ptr<void> tail;
   be_val<uint16_t> count;
   be_val<uint16_t> unk1; // TODO: Reverse me!
};
CHECK_OFFSET(MemoryList, 0x0, head);
CHECK_OFFSET(MemoryList, 0x4, tail);
CHECK_OFFSET(MemoryList, 0x8, count);
CHECK_OFFSET(MemoryList, 0xa, unk1);
CHECK_SIZE(MemoryList, 0xc);

#pragma pack(pop)

void
MEMInitList(MemoryList* list, uint16_t unk1); // MemoryList::unk1 = unk1

void
MEMAppendListObject(MemoryList* list, void *object);

void
MEMPrependListObject(MemoryList* list, void *object);

// TODO: unk1 is probably object to insert before / after
void
MEMInsertListObject(MemoryList* list, void *unk1, void* object);

void
MEMRemoveListObject(MemoryList* list, void* object);

void*
MEMGetNextListObject(MemoryList* list, void* object);

void*
MEMGetPrevListObject(MemoryList* list, void* object);

void*
MEMGetNthListObject(MemoryList* list, uint16_t n);
