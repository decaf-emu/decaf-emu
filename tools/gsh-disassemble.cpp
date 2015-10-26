#include <cassert>
#include <fstream>
#include <cstdint>
#include <string>
#include <vector>
#include <iostream>
#include "gpu/latte.h"
#include "gpu/hlsl/hlsl.h"
#include "gpu/hlsl/hlsl_generator.h"
#include "utils/be_val.h"
#include "utils/binaryfile.h"
#include "utils/strutils.h"

namespace gsh
{

struct Header
{
   static const uint32_t Magic = 0x47667832;
   be_val<uint32_t> magic;
   be_val<uint32_t> unk1;
   be_val<uint32_t> unk2;
   be_val<uint32_t> unk3;
   be_val<uint32_t> unk4;
   be_val<uint32_t> unk5;
   be_val<uint32_t> unk6;
   be_val<uint32_t> unk7;
};

struct Block
{
   enum BlockType : uint32_t
   {
      VertexShader = 5,
      PixelShader = 7
   };

   static const uint32_t Magic = 0x424C4B7B;
   be_val<uint32_t> magic;
   be_val<uint32_t> unk1;
   be_val<uint32_t> unk2;
   be_val<uint32_t> unk3;
   be_val<uint32_t> type;
   be_val<uint32_t> dataLength;
   be_val<uint32_t> unk4;
   be_val<uint32_t> unk5;
   gsl::array_view<uint8_t> data;
};

}

static bool
dumpShader(latte::Shader::Type type, const gsl::array_view<uint8_t> &data)
{
   std::string out;
   latte::disassemble(out, data);
   std::cout << "----------------------------------------------" << std::endl;
   std::cout << "                  Disassembly                 " << std::endl;
   std::cout << "----------------------------------------------" << std::endl;
   std::cout << out << std::endl;
   std::cout << std::endl;

   latte::Shader shader;
   latte::decode(shader, type, data);
   std::cout << "----------------------------------------------" << std::endl;
   std::cout << "                    Blocks                    " << std::endl;
   std::cout << "----------------------------------------------" << std::endl;
   latte::dumpBlocks(shader);
   std::cout << std::endl;

   std::string hlsl;
   hlsl::generateBody(shader, hlsl);
   std::cout << "----------------------------------------------" << std::endl;
   std::cout << "                     HLSL                     " << std::endl;
   std::cout << "----------------------------------------------" << std::endl;
   std::cout << hlsl << std::endl;
   std::cout << std::endl;
   return true;
}

static bool
parseGSH(BinaryFile &fh)
{
   gsh::Header header;
   fh.read(header.magic);

   if (header.magic != gsh::Header::Magic) {
      std::cout << "Unexpected gsh magic " << header.magic << std::endl;
      return false;
   }

   fh.read(header.unk1);
   fh.read(header.unk2);
   fh.read(header.unk3);
   fh.read(header.unk4);
   fh.read(header.unk5);
   fh.read(header.unk6);
   fh.read(header.unk7);

   while (!fh.eof()) {
      gsh::Block block;
      fh.read(block.magic);

      if (block.magic != gsh::Block::Magic) {
         std::cout << "Unexpected block magic " << header.magic << std::endl;
         return false;
      }

      fh.read(block.unk1);
      fh.read(block.unk2);
      fh.read(block.unk3);
      fh.read(block.type);
      fh.read(block.dataLength);
      fh.read(block.unk4);
      fh.read(block.unk5);

      block.data = fh.readView(block.dataLength);

      if (block.type == gsh::Block::VertexShader) {
         dumpShader(latte::Shader::Vertex, block.data);
      } else if (block.type == gsh::Block::PixelShader) {
         dumpShader(latte::Shader::Pixel, block.data);
      }
   }

   return true;
}

int main(int argc, char **argv)
{
   if (argc >= 3) {
      std::string strType = argv[1];
      std::string filename = argv[2];
      BinaryFile file;

      if (!file.open(filename)) {
         return -1;
      }

      if (strType.compare("gsh") == 0) {
         return parseGSH(file) ? 0 : -1;
      } else if (strType.compare("vertex") == 0) {
         return dumpShader(latte::Shader::Vertex, file) ? 1 : 0;
      } else if (strType.compare("pixel") == 0) {
         return dumpShader(latte::Shader::Pixel, file) ? 1 : 0;
      }
   }

   std::cout << "Usage: " << argv[0] << " <gsh|vertex|pixel> <filename>" << std::endl;
   return -1;
}

// Appease the compiler gods.
void *
memory_translate(ppcaddr_t address)
{
   return nullptr;
}

ppcaddr_t
memory_untranslate(const void *pointer)
{
   return 0;
}
