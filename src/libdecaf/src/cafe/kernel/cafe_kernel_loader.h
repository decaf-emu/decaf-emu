#include "cafe_kernel_process.h"
#include "cafe/loader/cafe_loader_minfileinfo.h"

#include <cstdint>

namespace cafe::kernel
{

int32_t
loaderLink(loader::LOADER_Handle handle,
           virt_ptr<loader::LOADER_MinFileInfo> minFileInfo,
           virt_ptr<loader::LOADER_LinkInfo> linkInfo,
           uint32_t linkInfoSize);

int32_t
loaderPrep(virt_ptr<loader::LOADER_MinFileInfo> minFileInfo);

int32_t
loaderPurge(loader::LOADER_Handle handle);

int32_t
loaderSetup(loader::LOADER_Handle handle,
            virt_ptr<loader::LOADER_MinFileInfo> minFileInfo);

int32_t
loaderQuery(loader::LOADER_Handle handle,
            virt_ptr<loader::LOADER_MinFileInfo> outMinFileInfo);

int32_t
loaderUserGainControl();

int32_t
findClosestSymbol(virt_addr addr,
                  virt_ptr<uint32_t> outSymbolDistance,
                  virt_ptr<char> symbolNameBuffer,
                  uint32_t symbolNameBufferLength,
                  virt_ptr<char> moduleNameBuffer,
                  uint32_t moduleNameBufferLength);

namespace internal
{

void
KiRPLStartup(UniqueProcessId callerProcessId,
             UniqueProcessId targetProcessId,
             ProcessFlags processFlags,
             uint32_t numCodeAreaHeapBlocks,
             uint32_t maxCodeSize,
             uint32_t maxDataSize,
             uint32_t titleLoc);

int32_t
findClosestSymbol(virt_addr addr,
                  virt_ptr<uint32_t> outSymbolDistance,
                  virt_ptr<char> symbolNameBuffer,
                  uint32_t symbolNameBufferLength,
                  virt_ptr<char> moduleNameBuffer,
                  uint32_t moduleNameBufferLength);

int32_t
findClosestSymbol(virt_addr addr,
                  uint32_t *outSymbolDistance,
                  char *symbolNameBuffer,
                  uint32_t symbolNameBufferLength,
                  char *moduleNameBuffer,
                  uint32_t moduleNameBufferLength);

} // namespace internal

} // namespace cafe::kernel::internal
