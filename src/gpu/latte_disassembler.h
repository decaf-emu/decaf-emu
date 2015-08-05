#pragma once
#include <cstdint>
#include <vector>
#include <spdlog/spdlog.h>
#include "latte.h"

namespace latte
{

class Disassembler
{
public:
   bool disassemble(uint8_t *binary, uint32_t size);

protected:
   void increaseIndent();
   void decreaseIndent();
   bool cfNormal(fmt::MemoryWriter &out, latte::cf::inst id, latte::cf::Instruction &cf);
   bool cfExport(fmt::MemoryWriter &out, latte::cf::inst id, latte::cf::Instruction &cf);
   bool cfALU(fmt::MemoryWriter &out, latte::cf::inst id, latte::cf::Instruction &cf);
   bool cfTEX(fmt::MemoryWriter &out, latte::cf::inst id, latte::cf::Instruction &cf);

private:
   std::string mIndent;
   uint32_t *mWords;
   uint32_t mWordCount;
   uint32_t mGroupCounter;
   uint32_t mControlFlowCounter;
};

} // namespace latte
