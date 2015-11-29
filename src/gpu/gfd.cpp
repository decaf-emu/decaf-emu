#include "gfd.h"
#include "utils/binaryfile.h"

namespace gfd
{

const uint32_t
FileHeader::Magic;

const uint32_t
BlockHeader::Magic;

bool
readFile(const std::string &filename, File &out)
{
   BinaryFile file;

   if (!file.open(filename)) {
      return false;
   }

   file.read(out.header);

   if (out.header.magic != FileHeader::Magic) {
      return false;
   }

   if (out.header.headerSize != sizeof(FileHeader)) {
      return false;
   }

   while (!file.eof()) {
      Block block;
      file.read(block.header);

      if (block.header.magic != BlockHeader::Magic) {
         return false;
      }

      if (block.header.headerSize != sizeof(BlockHeader)) {
         return false;
      }

      if (block.header.dataSize) {
         file.read(block.data, block.header.dataSize);
      }

      out.blocks.emplace_back(block);
   }

   return true;
}

} // namespace gsh
