#pragma once
#include "ios_auxil_enum.h"
#include "ios/ios_enum.h"

#include <libcpu/be2_struct.h>
#include <string_view>

namespace ios::auxil::internal
{

Error
UCInitFSA();

phys_ptr<uint8_t>
UCAllocFileData(uint32_t size);

void
UCFreeFileData(phys_ptr<uint8_t> fileData,
               uint32_t size);

UCError
UCReadConfigFile(std::string_view filename,
                 uint32_t *outBytesRead,
                 phys_ptr<uint8_t> *outBuffer);

UCError
UCWriteConfigFile(std::string_view filename,
                  phys_ptr<uint8_t> buffer,
                  uint32_t size);

} // namespace ios::auxil::internal
