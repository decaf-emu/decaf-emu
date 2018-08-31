#pragma once
#include "cafe_loader_basics.h"
#include "cafe_loader_rpl.h"
#include <libcpu/be2_struct.h>

namespace cafe::loader::internal
{

struct LiImportTracking
{
   be2_val<uint32_t> numExports;
   be2_virt_ptr<rpl::Export> exports;
   be2_val<uint32_t> tlsModuleIndex;
   be2_virt_ptr<LOADED_RPL> rpl;
};
CHECK_OFFSET(LiImportTracking, 0x00, numExports);
CHECK_OFFSET(LiImportTracking, 0x04, exports);
CHECK_OFFSET(LiImportTracking, 0x08, tlsModuleIndex);
CHECK_OFFSET(LiImportTracking, 0x0C, rpl);
CHECK_SIZE(LiImportTracking, 0x10);

int32_t
LiFixupRelocOneRPL(virt_ptr<LOADED_RPL> rpl,
                   virt_ptr<LiImportTracking> imports,
                   uint32_t unk);

} // namespace cafe::loader::internal;
