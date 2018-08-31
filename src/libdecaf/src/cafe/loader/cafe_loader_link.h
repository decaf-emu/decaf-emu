#pragma once
#include "cafe_loader_minfileinfo.h"

namespace cafe::loader::internal
{

int32_t
LOADER_Link(kernel::UniqueProcessId upid,
            virt_ptr<LOADER_LinkInfo> linkInfo,
            uint32_t linkInfoSize,
            virt_ptr<LOADER_MinFileInfo> minFileInfo);

} // namespace cafe::loader::internal
