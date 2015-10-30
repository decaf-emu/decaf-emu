#include <cstdint>
#include <vector>
#include "utils/be_val.h"

namespace gfd
{

#pragma pack(push, 1)

namespace BlockType
{
enum Type : uint32_t
{
   EndOfFile = 1,
   VertexShaderHeader = 3,
   VertexShaderProgram = 5,
   PixelShaderHeader = 6,
   PixelShaderProgram = 7,
   GeometryShaderHeader = 8,
   GeometryShaderProgram = 9,
   TextureHeader = 11,
   TextureImage = 12,
   TextureMipmap = 13,
};
}

struct FileHeader
{
   static const uint32_t Magic = 0x47667832;
   be_val<uint32_t> magic;
   be_val<uint32_t> headerSize;
   be_val<uint32_t> version1;
   be_val<uint32_t> version2;
   be_val<uint32_t> version3;
   be_val<uint32_t> align;
   be_val<uint32_t> unk1;
   be_val<uint32_t> unk2;
};

struct BlockHeader
{
   static const uint32_t Magic = 0x424C4B7B;
   be_val<uint32_t> magic;
   be_val<uint32_t> headerSize;
   be_val<uint32_t> version1;
   be_val<uint32_t> version2;
   be_val<BlockType::Type> type;
   be_val<uint32_t> dataSize;
   be_val<uint32_t> id;
   be_val<uint32_t> index;
};

#pragma pack(pop)

struct Block
{
   BlockHeader header;
   std::vector<uint8_t> data;
};

struct File
{
   FileHeader header;
   std::vector<Block> blocks;
};

bool
readFile(const std::string &filename, File &out);

} // namespace gfd

