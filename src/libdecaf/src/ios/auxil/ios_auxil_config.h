#pragma once
#include "ios_auxil_enum.h"
#include "ios/ios_ipc.h"

#include <cstdint>
#include <libcpu/be2_struct.h>
#include <common/structsize.h>
#include <string_view>

namespace ios::auxil
{

/**
 * \ingroup ios_auxil
 * @{
 */

#pragma pack(push, 1)

struct UCItem
{
   be2_array<char, 64> name;
   be2_val<uint32_t> access;
   be2_val<UCDataType> dataType;
   be2_val<UCError> error;
   be2_val<uint32_t> dataSize;
   be2_phys_ptr<void> data;
};
CHECK_OFFSET(UCItem, 0x00, name);
CHECK_OFFSET(UCItem, 0x40, access);
CHECK_OFFSET(UCItem, 0x44, dataType);
CHECK_OFFSET(UCItem, 0x48, error);
CHECK_OFFSET(UCItem, 0x4C, dataSize);
CHECK_OFFSET(UCItem, 0x50, data);
CHECK_SIZE(UCItem, 0x54);

#pragma pack(pop)

Error
openFsaHandle();

Error
closeFsaHandle();

UCFileSys
getFileSys(std::string_view name);

std::string_view
getRootKey(std::string_view name);

UCError
readItems(std::string_view fileSysPath,
          phys_ptr<UCItem> items,
          uint32_t count,
          phys_ptr<IoctlVec> vecs);

UCError
writeItems(std::string_view fileSysPath,
           phys_ptr<UCItem> items,
           uint32_t count,
           phys_ptr<IoctlVec> vecs);

UCError
deleteItems(std::string_view fileSysPath,
            phys_ptr<UCItem> items,
            uint32_t count);

/** @} */

} // namespace ios::auxil
