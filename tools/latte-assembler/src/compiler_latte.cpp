#include "shader_compiler.h"

latte::SQ_ALU_VEC_BANK_SWIZZLE
parseAluBankSwizzle(peg::Ast &node)
{
   if (node.token == "SCL_210") {
      return static_cast<latte::SQ_ALU_VEC_BANK_SWIZZLE>(latte::SQ_ALU_SCL_BANK_SWIZZLE::SCL_210);
   } else if (node.token == "SCL_122") {
      return static_cast<latte::SQ_ALU_VEC_BANK_SWIZZLE>(latte::SQ_ALU_SCL_BANK_SWIZZLE::SCL_122);
   } else if (node.token == "SCL_212") {
      return static_cast<latte::SQ_ALU_VEC_BANK_SWIZZLE>(latte::SQ_ALU_SCL_BANK_SWIZZLE::SCL_212);
   } else if (node.token == "SCL_221") {
      return static_cast<latte::SQ_ALU_VEC_BANK_SWIZZLE>(latte::SQ_ALU_SCL_BANK_SWIZZLE::SCL_221);
   } else if (node.token == "VEC_012") {
      return latte::SQ_ALU_VEC_BANK_SWIZZLE::VEC_012;
   } else if (node.token == "VEC_021") {
      return latte::SQ_ALU_VEC_BANK_SWIZZLE::VEC_021;
   } else if (node.token == "VEC_120") {
      return latte::SQ_ALU_VEC_BANK_SWIZZLE::VEC_120;
   } else if (node.token == "VEC_102") {
      return latte::SQ_ALU_VEC_BANK_SWIZZLE::VEC_102;
   } else if (node.token == "VEC_201") {
      return latte::SQ_ALU_VEC_BANK_SWIZZLE::VEC_201;
   } else if (node.token == "VEC_210") {
      return latte::SQ_ALU_VEC_BANK_SWIZZLE::VEC_210;
   } else {
      throw parse_exception(fmt::format("{}:{} Invalid ALU bank swizzle {}", node.line, node.column, node.token));
   }
}

latte::SQ_INDEX_MODE
parseAluDstRelIndexMode(peg::Ast &node)
{
   if (node.token == "[AR.x]") {
      return latte::SQ_INDEX_MODE::AR_X;
   } else if (node.token == "[AR.y]") {
      return latte::SQ_INDEX_MODE::AR_Y;
   } else if (node.token == "[AR.z]") {
      return latte::SQ_INDEX_MODE::AR_Z;
   } else if (node.token == "[AR.w]") {
      return latte::SQ_INDEX_MODE::AR_W;
   } else if (node.token == "[AL]") {
      return latte::SQ_INDEX_MODE::LOOP;
   } else {
      throw parse_exception(fmt::format("{}:{} Invalid ALU dst rel index mode {}", node.line, node.column, node.token));
   }
}

latte::SQ_CHAN
parseChan(peg::Ast &node)
{
   switch (node.token[0]) {
   case 'x':
   case 'X':
      return latte::SQ_CHAN::X;
   case 'y':
   case 'Y':
      return latte::SQ_CHAN::Y;
   case 'z':
   case 'Z':
      return latte::SQ_CHAN::Z;
   case 'w':
   case 'W':
      return latte::SQ_CHAN::W;
   case 't':
   case 'T':
      return latte::SQ_CHAN::T;
   default:
      throw parse_exception { fmt::format("{}:{} Invalid CHAN {}", node.line, node.column, node.token[0]) };
   }
}

latte::SQ_ALU_EXECUTE_MASK_OP
parseExecuteMaskOp(peg::Ast &node)
{
   if (!node.nodes.size()) {
      return latte::SQ_ALU_EXECUTE_MASK_OP::DEACTIVATE;
   } else if (node.nodes[0]->token == "DEACTIVATE") {
      return latte::SQ_ALU_EXECUTE_MASK_OP::DEACTIVATE;
   } else if (node.nodes[0]->token == "BREAK") {
      return latte::SQ_ALU_EXECUTE_MASK_OP::BREAK;
   } else if (node.nodes[0]->token == "CONTINUE") {
      return latte::SQ_ALU_EXECUTE_MASK_OP::CONTINUE;
   } else if (node.nodes[0]->token == "KILL") {
      return latte::SQ_ALU_EXECUTE_MASK_OP::KILL;
   } else {
      throw parse_exception(fmt::format("{}:{} Invalid ALU execute mask op {}", node.nodes[0]->line, node.nodes[0]->column, node.nodes[0]->token));
   }
}

size_t
parseFourCompSwizzle(peg::Ast &node,
                     latte::SQ_SEL &selX,
                     latte::SQ_SEL &selY,
                     latte::SQ_SEL &selZ,
                     latte::SQ_SEL &selW)
{
   assert(node.is_token);
   size_t numSel = 0;

   if (node.token.size() > 0) {
      selX = parseSel(node, 0);
      numSel++;
   }

   if (node.token.size() > 1) {
      selY = parseSel(node, 1);
      numSel++;
   }

   if (node.token.size() > 2) {
      selZ = parseSel(node, 2);
      numSel++;
   }

   if (node.token.size() > 3) {
      selW = parseSel(node, 3);
      numSel++;
   }

   return numSel;
}

latte::SQ_ALU_OMOD
parseOutputModifier(peg::Ast &node)
{
   if (node.token == "/2") {
      return latte::SQ_ALU_OMOD::D2;
   } else if (node.token == "*2") {
      return latte::SQ_ALU_OMOD::M2;
   } else if (node.token == "*4") {
      return latte::SQ_ALU_OMOD::M4;
   } else {
      throw parse_exception(fmt::format("{}:{} Invalid output modifier {}", node.line, node.column, node.token));
   }
}

latte::SQ_PRED_SEL
parsePredSel(peg::Ast &node)
{
   if (node.token == "PRED_SEL_OFF") {
      return latte::SQ_PRED_SEL::OFF;
   } else if (node.token == "PRED_SEL_ZERO") {
      return latte::SQ_PRED_SEL::ZERO;
   } else if (node.token == "PRED_SEL_ONE") {
      return latte::SQ_PRED_SEL::ONE;
   } else {
      throw parse_exception(fmt::format("{}:{} Invalid ALU pred sel {}", node.line, node.column, node.token));
   }
}

latte::SQ_SEL
parseSel(peg::Ast &node,
         unsigned index)
{
   switch (node.token[index]) {
   case 'x':
   case 'X':
      return latte::SQ_SEL::SEL_X;
   case 'y':
   case 'Y':
      return latte::SQ_SEL::SEL_Y;
   case 'z':
   case 'Z':
      return latte::SQ_SEL::SEL_Z;
   case 'w':
   case 'W':
      return latte::SQ_SEL::SEL_W;
   case '0':
      return latte::SQ_SEL::SEL_0;
   case '1':
      return latte::SQ_SEL::SEL_1;
   case '_':
      return latte::SQ_SEL::SEL_MASK;
   default:
      throw parse_exception { fmt::format("{}:{} Invalid SEL {}", node.line, node.column + index, node.token[index]) };
   }
}
