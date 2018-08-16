#pragma once
#include "cafe_loader_minfileinfo.h"

namespace cafe::loader::internal
{

virt_ptr<LOADED_RPL>
getModule(LOADER_Handle handle);

int32_t
LOADER_GetSecInfo(kernel::UniqueProcessId upid,
                  LOADER_Handle handle,
                  virt_ptr<uint32_t> outNumberOfSections,
                  virt_ptr<LOADER_SectionInfo> outSectionInfo);

int32_t
LOADER_GetFileInfo(kernel::UniqueProcessId upid,
                   LOADER_Handle handle,
                   virt_ptr<uint32_t> outSizeOfFileInfo,
                   virt_ptr<LOADER_UserFileInfo> outFileInfo,
                   virt_ptr<uint32_t> nextTlsModuleNumber,
                   virt_ptr<uint32_t> outFileLocation);

int32_t
LOADER_GetPathString(kernel::UniqueProcessId upid,
                     LOADER_Handle handle,
                     virt_ptr<uint32_t> outPathStringSize,
                     virt_ptr<char> pathStringBuffer,
                     virt_ptr<LOADER_UserFileInfo> outFileInfo);

int32_t
LOADER_Query(kernel::UniqueProcessId upid,
             LOADER_Handle handle,
             virt_ptr<LOADER_MinFileInfo> minFileInfo);

} // namespace cafe::loader::internal
