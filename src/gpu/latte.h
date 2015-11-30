#pragma once
#include <cstdint>
#include <gsl.h>
#include <memory>
#include <string>
#include <unordered_set>
#include "latte_shadir.h"

namespace latte
{

struct Shader
{
   enum Type
   {
      Geometry,
      Vertex,
      Pixel,
   };

   Type type;

   std::vector<std::unique_ptr<shadir::Instruction>> code;  // Serialised list of instructions
   std::vector<std::unique_ptr<shadir::Block>> blocks;      // Instruction AST
   std::vector<shadir::ExportInstruction *> exports;        // [Non-owning] list of exports

   std::unordered_set<uint32_t> pvUsed;         // Set of Previous Vectors used
   std::unordered_set<uint32_t> psUsed;         // Set of Previous Scalars used
   std::unordered_set<uint32_t> gprsUsed;       // Set of General Puprose Registers used
   std::unordered_set<uint32_t> uniformsUsed;   // Set of Uniforms used
   std::unordered_set<uint32_t> samplersUsed;   // Set of Sampler IDs used
   std::unordered_set<uint32_t> resourcesUsed;  // Set of Resource IDs used
};

bool decode(Shader &shader, Shader::Type type, const gsl::span<uint8_t> &binary);
bool blockify(Shader &shader);
bool analyse(Shader &shader);
void dumpBlocks(Shader &shader);
bool disassemble(std::string &out, const gsl::span<uint8_t> &binary);

} // namespace latte
