#pragma once
#include <array_view.h>
#include <cstdint>
#include <string>
#include <memory>
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
   }

   std::vector<shadir::ExportInstruction *> exports; // [Non-owning] list of exports
   std::vector<std::unique_ptr<shadir::Instruction>> code; // Serialised list of instructions
   std::vector<std::unique_ptr<shadir::Block>> blocks; // Instruction AST
};

bool decode(Shader &shader, const gsl::array_view<uint8_t> &binary);
bool blockify(Shader &shader);
bool disassemble(std::string &out, const gsl::array_view<uint8_t> &binary);
bool generateHLSL(Shader &shader, std::string &hlsl);

} // namespace latte
