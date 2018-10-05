#pragma once
#include "shader.h"

#include <fmt/format.h>
#include <peglib.h>
#include <stdexcept>
#include <string>
#include <vector>

class parse_exception : public std::runtime_error
{
public:
   parse_exception(const std::string &m) :
      std::runtime_error { m }
   {
   }
};

class node_parse_exception : public parse_exception
{
public:
   node_parse_exception(peg::Ast &node, const std::string &m) :
      parse_exception { fmt::format("{}:{} {}", node.line, node.column, m) }
   {
   }
};

class unhandled_node_exception : public node_parse_exception
{
public:
   unhandled_node_exception(peg::Ast &node) :
      node_parse_exception { node, fmt::format("Unxpected node {}", node.name) }
   {
   }
};

class invalid_inst_exception : public node_parse_exception
{
public:
   invalid_inst_exception(peg::Ast &node, const std::string &instType) :
      node_parse_exception { node, fmt::format("Invalid {} instruction {}", instType, node.token) }
   {
   }
};

class invalid_alu_op2_inst_exception : public invalid_inst_exception
{
public:
   invalid_alu_op2_inst_exception(peg::Ast &node) :
      invalid_inst_exception { node, "ALU OP2" }
   {
   }
};

class invalid_alu_op3_inst_exception : public invalid_inst_exception
{
public:
   invalid_alu_op3_inst_exception(peg::Ast &node) :
      invalid_inst_exception { node, "ALU OP3" }
   {
   }
};

class invalid_cf_inst_exception : public invalid_inst_exception
{
public:
   invalid_cf_inst_exception(peg::Ast &node) :
      invalid_inst_exception { node, "CF" }
   {
   }
};

class invalid_cf_alu_inst_exception : public invalid_inst_exception
{
public:
   invalid_cf_alu_inst_exception(peg::Ast &node) :
      invalid_inst_exception { node, "CF ALU" }
   {
   }
};

class invalid_cf_tex_inst_exception : public invalid_inst_exception
{
public:
   invalid_cf_tex_inst_exception(peg::Ast &node) :
      invalid_inst_exception { node, "CF TEX" }
   {
   }
};

class invalid_exp_inst_exception : public invalid_inst_exception
{
public:
   invalid_exp_inst_exception(peg::Ast &node) :
      invalid_inst_exception { node, "EXP" }
   {
   }
};

class invalid_tex_inst_exception : public invalid_inst_exception
{
public:
   invalid_tex_inst_exception(peg::Ast &node) :
      invalid_inst_exception { node, "TEX" }
   {
   }
};

class invalid_inst_property_exception : public node_parse_exception
{
public:
   invalid_inst_property_exception(peg::Ast &node, const std::string &instType) :
      node_parse_exception { node, fmt::format("Invalid property {} for {} instruction", node.name, instType) }
   {
   }
};

class invalid_alu_property_exception : public invalid_inst_property_exception
{
public:
   invalid_alu_property_exception(peg::Ast &node) :
      invalid_inst_property_exception { node, "ALU" }
   {
   }
};

class invalid_cf_property_exception : public invalid_inst_property_exception
{
public:
   invalid_cf_property_exception(peg::Ast &node) :
      invalid_inst_property_exception { node, "CF" }
   {
   }
};

class invalid_cf_alu_property_exception : public invalid_inst_property_exception
{
public:
   invalid_cf_alu_property_exception(peg::Ast &node) :
      invalid_inst_property_exception { node, "CF ALU" }
   {
   }
};

class invalid_cf_tex_property_exception : public invalid_inst_property_exception
{
public:
   invalid_cf_tex_property_exception(peg::Ast &node) :
      invalid_inst_property_exception { node, "CF TEX" }
   {
   }
};

class invalid_exp_property_exception : public invalid_inst_property_exception
{
public:
   invalid_exp_property_exception(peg::Ast &node) :
      invalid_inst_property_exception { node, "EXP" }
   {
   }
};

class invalid_tex_property_exception : public invalid_inst_property_exception
{
public:
   invalid_tex_property_exception(peg::Ast &node) :
      invalid_inst_property_exception { node, "TEX" }
   {
   }
};

class incorrect_cf_pc_exception : public node_parse_exception
{
public:
   incorrect_cf_pc_exception(peg::Ast &node, size_t found, size_t expected) :
      node_parse_exception { node, fmt::format("Incorrect CF PC {}, expected {}", found, expected) }
   {
   }
};

class incorrect_clause_pc_exception : public node_parse_exception
{
public:
   incorrect_clause_pc_exception(peg::Ast &node, size_t found, size_t expected) :
      node_parse_exception { node, fmt::format("Incorrect clause PC {}, expected {}", found, expected) }
   {
   }
};

class incorrect_clause_addr_exception : public node_parse_exception
{
public:
   incorrect_clause_addr_exception(peg::Ast &node, size_t found, size_t expected) :
      node_parse_exception { node, fmt::format("Incorrect clause addr {}, expected {}", found, expected) }
   {
   }
};

class incorrect_clause_count_exception : public node_parse_exception
{
public:
   incorrect_clause_count_exception(peg::Ast &node, size_t found, size_t expected) :
      node_parse_exception { node, fmt::format("Incorrect clause count {}, expected {}", found, expected) }
   {
   }
};

// compiler_parse
bool
compileShaderCode(Shader &shader,
                  std::vector<char> &code);

// compiler_cf
void
compileAST(Shader &shader,
           std::shared_ptr<peg::Ast> ast);

// compiler_alu
void
compileAluClause(Shader &shader,
                 peg::Ast &node);

// compiler_exp
void
compileExpInst(Shader &shader,
               peg::Ast &node);

// compiler_tex
void
compileTexClause(Shader &shader,
                 peg::Ast &node);

// compiler_latte
latte::SQ_ALU_VEC_BANK_SWIZZLE
parseAluBankSwizzle(peg::Ast &node);

latte::SQ_INDEX_MODE
parseAluDstRelIndexMode(peg::Ast &node);

latte::SQ_CF_COND
parseCfCond(peg::Ast &node);

latte::SQ_CHAN
parseChan(peg::Ast &node);

size_t
parseFourCompSwizzle(peg::Ast &node,
                     latte::SQ_SEL &selX,
                     latte::SQ_SEL &selY,
                     latte::SQ_SEL &selZ,
                     latte::SQ_SEL &selW);

latte::SQ_ALU_OMOD
parseOutputModifier(peg::Ast &node);

latte::SQ_PRED_SEL
parsePredSel(peg::Ast &node);

latte::SQ_SEL
parseSel(peg::Ast &node,
         unsigned index);

// compiler_common
float
parseFloat(peg::Ast &node);

uint32_t
parseHexNumber(peg::Ast &node);

unsigned long
parseNumber(peg::Ast &node);

LiteralValue
parseLiteral(peg::Ast &node);

void
markGprRead(Shader &shader,
            uint32_t gpr);

void
markSrcRead(Shader &shader,
            latte::SQ_ALU_SRC src);

void
markGprWritten(Shader &shader,
               uint32_t gpr);
