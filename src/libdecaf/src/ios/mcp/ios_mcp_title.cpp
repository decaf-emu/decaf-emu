#include "ios_mcp_config.h"
#include "ios_mcp_title.h"

#include "ios/ios_stackobject.h"
#include "ios/auxil/ios_auxil_config.h"
#include "ios/kernel/ios_kernel_process.h"

using namespace ios::auxil;
using namespace ios::kernel;

namespace ios::mcp::internal
{

struct StaticTitleData
{
   be2_struct<MCPPPrepareTitleInfo> prepareTitleInfoBuffer;
};

static phys_ptr<StaticTitleData>
sTitleData = nullptr;

static MCPError
readTitleConfigItems(std::string_view path,
                     phys_ptr<UCItem> items,
                     uint32_t count)
{
   return translateUCError(readItemsFromFile(path, items, count, nullptr));
}

phys_ptr<MCPPPrepareTitleInfo>
getPrepareTitleInfoBuffer()
{
   return phys_addrof(sTitleData->prepareTitleInfoBuffer);
}

MCPError
readTitleCosXml(phys_ptr<MCPPPrepareTitleInfo> titleInfo)
{
   StackArray<UCItem, 60> items;
   items[0].name = "app";
   items[0].access = 0x777u;
   items[0].dataType = UCDataType::Complex;

   items[1].name = "app.version";
   items[1].dataType = UCDataType::UnsignedInt;
   items[1].dataSize = 4u;
   items[1].data = phys_addrof(titleInfo->version);

   items[2].name = "app.cmdFlags";
   items[2].dataType = UCDataType::UnsignedInt;
   items[2].dataSize = 4u;
   items[2].data = phys_addrof(titleInfo->cmdFlags);

   items[3].name = "app.argstr";
   items[3].dataType = UCDataType::String;
   items[3].dataSize = 4096u;
   items[3].data = phys_addrof(titleInfo->argstr);

   items[4].name = "app.max_size";
   items[4].dataType = UCDataType::HexBinary;
   items[4].dataSize = 4u;
   items[4].data = phys_addrof(titleInfo->max_size);

   items[5].name = "app.avail_size";
   items[5].dataType = UCDataType::HexBinary;
   items[5].dataSize = 4u;
   items[5].data = phys_addrof(titleInfo->avail_size);

   items[6].name = "app.codegen_size";
   items[6].dataType = UCDataType::HexBinary;
   items[6].dataSize = 4u;
   items[6].data = phys_addrof(titleInfo->codegen_size);

   items[7].name = "app.codegen_core";
   items[7].dataType = UCDataType::HexBinary;
   items[7].dataSize = 4u;
   items[7].data = phys_addrof(titleInfo->codegen_core);

   items[8].name = "app.max_codesize";
   items[8].dataType = UCDataType::HexBinary;
   items[8].dataSize = 4u;
   items[8].data = phys_addrof(titleInfo->max_codesize);

   items[9].name = "app.overlay_arena";
   items[9].dataType = UCDataType::HexBinary;
   items[9].dataSize = 4u;
   items[9].data = phys_addrof(titleInfo->overlay_arena);

   items[10].name = "app.num_workarea_heap_blocks";
   items[10].dataType = UCDataType::UnsignedInt;
   items[10].dataSize = 4u;
   items[10].data = phys_addrof(titleInfo->num_workarea_heap_blocks);

   items[11].name = "app.num_codearea_heap_blocks";
   items[11].dataType = UCDataType::UnsignedInt;
   items[11].dataSize = 4u;
   items[11].data = phys_addrof(titleInfo->num_codearea_heap_blocks);

   items[12].name = "app.permissions";
   items[12].dataType = UCDataType::Complex;

   for (auto i = 0u; i <= 18; ++i) {
      auto &mask = items[13 + (i * 2)];
      *fmt::format_to(phys_addrof(mask.name).get(),
                      "app.permissions.p{}.mask", i) = char { 0 };
      mask.dataType = UCDataType::HexBinary;
      mask.dataSize = 8u;
      mask.data = phys_addrof(titleInfo->permissions[i].mask);

      auto &group = items[14 + (i * 2)];
      *fmt::format_to(phys_addrof(group.name).get(),
                      "app.permissions.p{}.group", i) = char { 0 };
      group.dataType = UCDataType::UnsignedInt;
      group.dataSize = 4u;
      group.data = phys_addrof(titleInfo->permissions[i].group);
   }

   items[51].name = "app.default_stack0_size";
   items[51].dataType = UCDataType::HexBinary;
   items[51].dataSize = 4u;
   items[51].data = phys_addrof(titleInfo->default_stack0_size);

   items[52].name = "app.default_stack1_size";
   items[52].dataType = UCDataType::HexBinary;
   items[52].dataSize = 4u;
   items[52].data = phys_addrof(titleInfo->default_stack1_size);

   items[53].name = "app.default_stack2_size";
   items[53].dataType = UCDataType::HexBinary;
   items[53].dataSize = 4u;
   items[53].data = phys_addrof(titleInfo->default_stack2_size);

   items[54].name = "app.default_redzone0_size";
   items[54].dataType = UCDataType::HexBinary;
   items[54].dataSize = 4u;
   items[54].data = phys_addrof(titleInfo->default_redzone0_size);

   items[55].name = "app.default_redzone1_size";
   items[55].dataType = UCDataType::HexBinary;
   items[55].dataSize = 4u;
   items[55].data = phys_addrof(titleInfo->default_redzone1_size);

   items[56].name = "app.default_redzone2_size";
   items[56].dataType = UCDataType::HexBinary;
   items[56].dataSize = 4u;
   items[56].data = phys_addrof(titleInfo->default_redzone2_size);

   items[57].name = "app.exception_stack0_size";
   items[57].dataType = UCDataType::HexBinary;
   items[57].dataSize = 4u;
   items[57].data = phys_addrof(titleInfo->exception_stack0_size);

   items[58].name = "app.exception_stack1_size";
   items[58].dataType = UCDataType::HexBinary;
   items[58].dataSize = 4u;
   items[58].data = phys_addrof(titleInfo->exception_stack1_size);

   items[59].name = "app.exception_stack2_size";
   items[59].dataType = UCDataType::HexBinary;
   items[59].dataSize = 4u;
   items[59].data = phys_addrof(titleInfo->exception_stack2_size);

   auto error = readTitleConfigItems("/vol/code/cos.xml", items, items.size());
   if (error < MCPError::OK && error != MCPError::KeyNotFound) {
      // KeyNotFound is allowed because not all fields are required in xml
      return error;
   }

   return MCPError::OK;
}

void
initialiseTitleStaticData()
{
   sTitleData = allocProcessStatic<StaticTitleData>();
}

} // namespace ios::mcp::internal
