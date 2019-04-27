#pragma once
#include "coreinit_enum.h"

#include <libcpu/be2_struct.h>

namespace cafe::coreinit
{

struct OSThread;
struct OSDynLoad_NotifyData;

using OSDynLoad_ModuleHandle = uint32_t;
constexpr OSDynLoad_ModuleHandle
OSDynLoad_CurrentModuleHandle = OSDynLoad_ModuleHandle { 0xFFFFFFFF };

using OSDynLoad_AllocFn = virt_func_ptr<
   OSDynLoad_Error(int32_t size,
                   int32_t align,
                   virt_ptr<virt_ptr<void>> out)>;

using OSDynLoad_FreeFn = virt_func_ptr<
   void(virt_ptr<void> ptr)>;

using OSDynLoad_NotifyCallbackFn = virt_func_ptr<
   void(OSDynLoad_ModuleHandle handle,
        virt_ptr<void> userArg1,
        OSDynLoad_NotifyEvent event,
        virt_ptr<OSDynLoad_NotifyData>)>;

struct OSDynLoad_LoaderHeapStatistics;

struct OSDynLoad_NotifyData
{
   be2_virt_ptr<char> name;

   be2_val<virt_addr> textAddr;
   be2_val<uint32_t> textOffset;
   be2_val<uint32_t> textSize;

   be2_val<virt_addr> dataAddr;
   be2_val<uint32_t> dataOffset;
   be2_val<uint32_t> dataSize;

   be2_val<virt_addr> readAddr;
   be2_val<uint32_t> readOffset;
   be2_val<uint32_t> readSize;
};
CHECK_OFFSET(OSDynLoad_NotifyData, 0x00, name);
CHECK_OFFSET(OSDynLoad_NotifyData, 0x04, textAddr);
CHECK_OFFSET(OSDynLoad_NotifyData, 0x08, textOffset);
CHECK_OFFSET(OSDynLoad_NotifyData, 0x0C, textSize);
CHECK_OFFSET(OSDynLoad_NotifyData, 0x10, dataAddr);
CHECK_OFFSET(OSDynLoad_NotifyData, 0x14, dataOffset);
CHECK_OFFSET(OSDynLoad_NotifyData, 0x18, dataSize);
CHECK_OFFSET(OSDynLoad_NotifyData, 0x1C, readAddr);
CHECK_OFFSET(OSDynLoad_NotifyData, 0x20, readOffset);
CHECK_OFFSET(OSDynLoad_NotifyData, 0x24, readSize);
CHECK_SIZE(OSDynLoad_NotifyData, 0x28);

struct OSDynLoad_NotifyCallback
{
   UNKNOWN(0x4);
   be2_val<OSDynLoad_NotifyCallbackFn> notifyFn;
   be2_virt_ptr<void> userArg1;
   be2_virt_ptr<OSDynLoad_NotifyCallback> next;
};
CHECK_SIZE(OSDynLoad_NotifyCallback, 0x10);

struct tls_index
{
   be2_val<uint32_t> moduleIndex;
   be2_val<uint32_t> offset;
};
CHECK_OFFSET(tls_index, 0x00, moduleIndex);
CHECK_OFFSET(tls_index, 0x04, offset);
CHECK_SIZE(tls_index, 0x08);

OSDynLoad_Error
OSDynLoad_AddNotifyCallback(OSDynLoad_NotifyCallbackFn notifyFn,
                            virt_ptr<void> userArg1);

void
OSDynLoad_DelNotifyCallback(OSDynLoad_NotifyCallbackFn notifyFn,
                            virt_ptr<void> userArg1);

OSDynLoad_Error
OSDynLoad_GetAllocator(virt_ptr<OSDynLoad_AllocFn> outAllocFn,
                       virt_ptr<OSDynLoad_FreeFn> outFreeFn);

OSDynLoad_Error
OSDynLoad_SetAllocator(OSDynLoad_AllocFn allocFn,
                       OSDynLoad_FreeFn freeFn);

OSDynLoad_Error
OSDynLoad_GetTLSAllocator(virt_ptr<OSDynLoad_AllocFn> outAllocFn,
                          virt_ptr<OSDynLoad_FreeFn> outFreeFn);

OSDynLoad_Error
OSDynLoad_SetTLSAllocator(OSDynLoad_AllocFn allocFn,
                          OSDynLoad_FreeFn freeFn);

OSDynLoad_Error
OSDynLoad_Acquire(virt_ptr<const char> modulePath,
                  virt_ptr<OSDynLoad_ModuleHandle> outModuleHandle);

OSDynLoad_Error
OSDynLoad_AcquireContainingModule(virt_ptr<void> ptr,
                                  OSDynLoad_SectionType sectionType,
                                  virt_ptr<OSDynLoad_ModuleHandle> outHandle);

OSDynLoad_Error
OSDynLoad_FindExport(OSDynLoad_ModuleHandle moduleHandle,
                     BOOL isData,
                     virt_ptr<const char> name,
                     virt_ptr<virt_addr> outAddr);

OSDynLoad_Error
OSDynLoad_FindTag(OSDynLoad_ModuleHandle moduleHandle,
                  virt_ptr<const char> tag,
                  virt_ptr<char> buffer,
                  virt_ptr<uint32_t> inoutBufferSize);

OSDynLoad_Error
OSDynLoad_GetLoaderHeapStatistics(virt_ptr<OSDynLoad_LoaderHeapStatistics> stats);

OSDynLoad_Error
OSDynLoad_GetModuleName(OSDynLoad_ModuleHandle moduleHandle,
                        virt_ptr<char> buffer,
                        virt_ptr<uint32_t> inoutBufferSize);

uint32_t
OSDynLoad_GetNumberOfRPLs();

uint32_t
OSDynLoad_GetRPLInfo(uint32_t first,
                     uint32_t count,
                     virt_ptr<OSDynLoad_NotifyData> outRplInfos);

OSDynLoad_Error
OSDynLoad_IsModuleLoaded(virt_ptr<const char> name,
                         virt_ptr<OSDynLoad_ModuleHandle> outHandle);

void
OSDynLoad_Release(OSDynLoad_ModuleHandle moduleHandle);

virt_addr
OSGetSymbolName(virt_addr address,
                virt_ptr<char> buffer,
                uint32_t bufferSize);

virt_ptr<void>
tls_get_addr(virt_ptr<tls_index> index);

namespace internal
{

void
dynLoadTlsFree(virt_ptr<OSThread> thread);

virt_addr
initialiseDynLoad();

OSDynLoad_Error
relocateHleLibrary(OSDynLoad_ModuleHandle moduleHandle);

} // namespace internal

} // namespace cafe::coreinit
