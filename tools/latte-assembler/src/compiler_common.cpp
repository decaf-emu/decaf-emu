#include "shader_compiler.h"

unsigned long
parseNumber(peg::Ast &node)
{
   assert(node.is_token);
   return std::stoul(node.token);
}

float
parseFloat(peg::Ast &node)
{
   return std::stof(node.token);
}

uint32_t
parseHexNumber(peg::Ast &node)
{
   return static_cast<uint32_t>(std::stoul(node.token, 0, 0));
}

LiteralValue
parseLiteral(peg::Ast &node)
{
   auto literal = LiteralValue { };

   for (auto child : node.nodes) {
      if (child->name == "HexNumber") {
         literal.flags |= LiteralValue::ReadHex;
         literal.hexValue = parseHexNumber(*child);
      } else if (child->name == "Float") {
         literal.flags |= LiteralValue::ReadFloat;
         literal.floatValue = parseFloat(*child);
      }
   }

   return literal;
}

void
markGprRead(Shader &shader,
            uint32_t gpr)
{
   shader.gprRead[gpr] = true;
}

void
markGprWritten(Shader &shader,
               uint32_t gpr)
{
   shader.gprWritten[gpr] = true;
}

void
markSrcRead(Shader &shader,
            latte::SQ_ALU_SRC src)
{
   if (src >= latte::SQ_ALU_SRC::REGISTER_FIRST && src <= latte::SQ_ALU_SRC::REGISTER_LAST) {
      markGprRead(shader, src - latte::SQ_ALU_SRC::REGISTER_FIRST);
   } else if (src >= latte::SQ_ALU_SRC::KCACHE_BANK0_FIRST && src <= latte::SQ_ALU_SRC::KCACHE_BANK0_LAST) {
      shader.uniformBlocksUsed = true;
   } else if (src >= latte::SQ_ALU_SRC::KCACHE_BANK1_FIRST && src <= latte::SQ_ALU_SRC::KCACHE_BANK1_LAST) {
      shader.uniformBlocksUsed = true;
   } else if (src >= latte::SQ_ALU_SRC::CONST_FILE_FIRST && src <= latte::SQ_ALU_SRC::CONST_FILE_LAST) {
      shader.uniformRegistersUsed = true;
   }
}
