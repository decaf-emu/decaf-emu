#pragma once
#include <libcpu/be2_struct.h>

namespace cafe::coreinit
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
   be2_virt_ptr<void> prev;
   be2_virt_ptr<void> next;
};

CHECK_OFFSET(MEMListLink, 0x0, prev);
CHECK_OFFSET(MEMListLink, 0x4, next);
CHECK_SIZE(MEMListLink, 0x8);

struct MEMList
{
   be2_virt_ptr<void> head;
   be2_virt_ptr<void> tail;
   be2_val<uint16_t> count;
   be2_val<uint16_t> offsetToMEMListLink;
};
CHECK_OFFSET(MEMList, 0x0, head);
CHECK_OFFSET(MEMList, 0x4, tail);
CHECK_OFFSET(MEMList, 0x8, count);
CHECK_OFFSET(MEMList, 0xa, offsetToMEMListLink);
CHECK_SIZE(MEMList, 0xc);

#pragma pack(pop)

void
MEMInitList(virt_ptr<MEMList> list,
            uint16_t offsetToMEMListLink);

void
MEMAppendListObject(virt_ptr<MEMList> list,
                    virt_ptr<void> object);

void
MEMPrependListObject(virt_ptr<MEMList> list,
                     virt_ptr<void> object);

void
MEMInsertListObject(virt_ptr<MEMList> list,
                    virt_ptr<void> before,
                    virt_ptr<void> object);

void
MEMRemoveListObject(virt_ptr<MEMList> list,
                    virt_ptr<void> object);

virt_ptr<void>
MEMGetNextListObject(virt_ptr<MEMList> list,
                     virt_ptr<void> object);

virt_ptr<void>
MEMGetPrevListObject(virt_ptr<MEMList> list,
                     virt_ptr<void> object);

virt_ptr<void>
MEMGetNthListObject(virt_ptr<MEMList> list,
                    uint16_t n);

/** @} */

} // namespace cafe::coreinit
