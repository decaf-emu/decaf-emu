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

struct MEMListLink
{
   be_ptr<void> prev;
   be_ptr<void> next;
};

CHECK_OFFSET(MEMListLink, 0x0, prev);
CHECK_OFFSET(MEMListLink, 0x4, next);
CHECK_SIZE(MEMListLink, 0x8);

struct MEMList
{
   be_ptr<void> head;
   be_ptr<void> tail;
   be_val<uint16_t> count;
   be_val<uint16_t> offsetToMEMListLink;
};
CHECK_OFFSET(MEMList, 0x0, head);
CHECK_OFFSET(MEMList, 0x4, tail);
CHECK_OFFSET(MEMList, 0x8, count);
CHECK_OFFSET(MEMList, 0xa, offsetToMEMListLink);
CHECK_SIZE(MEMList, 0xc);

#pragma pack(pop)

void
MEMInitList(MEMList *list,
            uint16_t offsetToMEMListLink);

void
MEMAppendListObject(MEMList *list,
                    void *object);

void
MEMPrependListObject(MEMList *list,
                     void *object);

void
MEMInsertListObject(MEMList *list,
                    void *before,
                    void *object);

void
MEMRemoveListObject(MEMList *list,
                    void *object);

void *
MEMGetNextListObject(MEMList *list,
                     void *object);

void *
MEMGetPrevListObject(MEMList *list,
                     void *object);

void *
MEMGetNthListObject(MEMList *list,
                    uint16_t n);

/** @} */

} // namespace coreinit
