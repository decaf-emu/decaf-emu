#pragma once
#include <cstdint>
#include <common/be_ptr.h>
#include <common/be_val.h>
#include <common/structsize.h>

namespace kernel
{

namespace ios
{

namespace usr_cfg
{

/**
 * \ingroup kernel_ios_usr_cfg
 * @{
 */

#pragma pack(push, 1)

struct UCSysConfig
{
   char name[64];
   be_val<uint32_t> access;
   be_val<UCDataType> dataType;
   be_val<UCError> error;
   be_val<uint32_t> dataSize;
   be_ptr<void> data;
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

} // namespace usr_cfg

} // namespace ios

} // namespace kernel
