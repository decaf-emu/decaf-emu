#pragma once
#include <cstdint>
#include <string>
#include "latte_shadir.h"

namespace latte
{

struct Shader
{
   Shader()
   {
   }

   ~Shader()
   {
      for (auto block : blocks) {
         delete block;
      }

      for (auto ins : code) {
         delete ins;
      }
   }

   std::vector<shadir::Instruction *> code; // Serialised list of instructions
   std::vector<shadir::Block *> blocks; // Instruction AST
};

bool decode(Shader &shader, const uint8_t *binary, uint32_t size);
bool blockify(Shader &shader);
bool disassemble(std::string &out, const uint8_t *binary, uint32_t size);
bool generateHLSL(Shader &shader, std::string &hlsl);

} // namespace latte
