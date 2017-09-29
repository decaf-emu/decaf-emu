#pragma once
#include "ios_auxil_enum.h"

#include <cstdint>
#include <libcpu/be2_struct.h>
#include <common/structsize.h>

namespace ios::auxil
{

/**
 * \ingroup ios_auxil
 * @{
 */

#pragma pack(push, 1)

struct UCSysConfig
{
   be2_array<char, 64> name;
   be2_val<uint32_t> access;
   be2_val<UCDataType> dataType;
   be2_val<UCError> error;
   be2_val<uint32_t> dataSize;
   be2_virt_ptr<void> data;
};
CHECK_OFFSET(UCSysConfig, 0x00, name);
CHECK_OFFSET(UCSysConfig, 0x40, access);
CHECK_OFFSET(UCSysConfig, 0x44, dataType);
CHECK_OFFSET(UCSysConfig, 0x48, error);
CHECK_OFFSET(UCSysConfig, 0x4C, dataSize);
CHECK_OFFSET(UCSysConfig, 0x50, data);
CHECK_SIZE(UCSysConfig, 0x54);

#pragma pack(pop)

/** @} */

} // namespace ios::auxil
