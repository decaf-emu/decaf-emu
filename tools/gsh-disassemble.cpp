#include <cassert>
#include <fstream>
#include <cstdint>
#include <string>
#include <vector>
#include <iostream>
#include "bigendianview.h"
#include "gpu/latte.h"

namespace gsh
{

struct Header
{
   static const uint32_t Magic = 0x47667832;
   uint32_t magic;
   uint32_t unk1;
   uint32_t unk2;
   uint32_t unk3;
   uint32_t unk4;
   uint32_t unk5;
   uint32_t unk6;
   uint32_t unk7;
};

struct Block
{
   enum BlockType : uint32_t
   {
      VertexShader = 5,
      PixelShader = 7
   };

   static const uint32_t Magic = 0x424c4b7b;
   uint32_t magic;
   uint32_t unk1;
   uint32_t unk2;
   uint32_t unk3;
   uint32_t type;
   uint32_t dataLength;
   uint32_t unk4;
   uint32_t unk5;
   std::vector<uint8_t> data;
};

}

bool parseGSH(BigEndianView &fh)
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

      block.data.resize(block.dataLength);
      fh.read<uint8_t>(block.data);

      if (block.type == gsh::Block::VertexShader || block.type == gsh::Block::PixelShader) {
         std::string out;
         latte::disassemble(out, block.data);
         std::cout << "----------------------------------------------" << std::endl;
         std::cout << "                  Disassembly                 " << std::endl;
         std::cout << "----------------------------------------------" << std::endl;
         std::cout << out << std::endl;
         std::cout << std::endl;

         latte::Shader shader;
         latte::decode(shader, block.data);
         std::cout << "----------------------------------------------" << std::endl;
         std::cout << "                    Blocks                    " << std::endl;
         std::cout << "----------------------------------------------" << std::endl;
         latte::blockify(shader);
         std::cout << std::endl;

         std::string hlsl;
         latte::generateHLSL(shader, hlsl);
         std::cout << "----------------------------------------------" << std::endl;
         std::cout << "                     HLSL                     " << std::endl;
         std::cout << "----------------------------------------------" << std::endl;
         std::cout << hlsl << std::endl;
         std::cout << std::endl;
      }
   }

   return true;
}

int main(int argc, char **argv)
{
   // Read whole file
   std::vector<char> data;
   std::ifstream file(argv[1], std::ifstream::binary | std::ifstream::in);
   file.seekg(0, std::ifstream::end);
   auto size = static_cast<size_t>(file.tellg());
   file.seekg(0, std::ifstream::beg);
   data.resize(size);
   file.read(data.data(), size);
   file.close();

   // Parse
   BigEndianView fh { data.data(), data.size() };
   return parseGSH(fh) ? 0 : -1;
}
