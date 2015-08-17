#pragma once
#include <cstdint>
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

bool decode(Shader &shader, uint8_t *binary, uint32_t size);
bool blockify(Shader &shader);

} // namespace latte
