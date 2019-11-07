#pragma once
#include "nn_cmpt_enum.h"
#include <libcpu/be2_struct.h>

namespace cafe::nn_cmpt
{

struct CMPTPcConf
{
   be2_val<uint32_t> rating;
   be2_val<uint32_t> organisation;
   be2_val<CMPTPcConfFlags> flags;
};
CHECK_OFFSET(CMPTPcConf, 0x00, rating);
CHECK_OFFSET(CMPTPcConf, 0x04, organisation);
CHECK_OFFSET(CMPTPcConf, 0x08, flags);
CHECK_SIZE(CMPTPcConf, 0x0C);

CMPTError
CMPTGetDataSize(virt_ptr<uint32_t> outDataSize);

CMPTError
CMPTAcctGetPcConf(virt_ptr<CMPTPcConf> outConf);

} // namespace cafe::nn_cmpt
