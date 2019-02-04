#pragma once
#include <libcpu/be2_struct.h>

namespace cafe::nn_boss
{

struct TitleID
{
   be2_val<uint64_t> value;
};
CHECK_OFFSET(TitleID, 0, value);
CHECK_SIZE(TitleID, 8);

virt_ptr<TitleID>
TitleID_Constructor(virt_ptr<TitleID> self);

virt_ptr<TitleID>
TitleID_Constructor(virt_ptr<TitleID> self,
                    virt_ptr<TitleID> other);

virt_ptr<TitleID>
TitleID_Constructor(virt_ptr<TitleID> self,
                    uint64_t id);

bool
TitleID_IsValid(virt_ptr<TitleID> self);

uint64_t
TitleID_GetValue(virt_ptr<TitleID> self);

uint32_t
TitleID_GetTitleID(virt_ptr<TitleID> self);

uint32_t
TitleID_GetTitleCode(virt_ptr<TitleID> self);

uint32_t
TitleID_GetUniqueId(virt_ptr<TitleID> self);

bool
TitleID_OperatorEqual(virt_ptr<TitleID> self,
                      virt_ptr<TitleID> other);

bool
TitleID_OperatorNotEqual(virt_ptr<TitleID> self,
                         virt_ptr<TitleID> other);

} // namespace cafe::nn_boss
