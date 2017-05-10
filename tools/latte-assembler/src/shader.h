#pragma once
#include <cstdint>
#include <libgpu/latte/latte_instructions.h>
#include <peglib.h>
#include <vector>
#include <string>

enum class ShaderType
{
   PixelShader,
   VertexShader,
};

struct LiteralValue
{
   enum Flags
   {
      ReadHex = 1 << 0,
      ReadFloat = 1 << 1,
   };

   unsigned flags;

   union
   {
      uint32_t hexValue;
      float floatValue;
   };
};

struct AluGroup
{
   uint32_t clausePC = 0;
   std::vector<latte::AluInst> insts;
   std::vector<LiteralValue> literals;
};

struct AluClause
{
   std::shared_ptr<peg::Ast> addrNode;
   std::shared_ptr<peg::Ast> countNode;
   uint32_t cfPC = 0;
   std::vector<AluGroup> groups;
};

struct Shader
{
   std::string path;

   ShaderType type;
   uint32_t clausePC = 0;
   std::vector<latte::ControlFlowInst> cfInsts;
   std::vector<AluClause> aluClauses;
   uint32_t aluClauseBaseAddress;
   std::vector<uint32_t> aluClauseData;
   std::vector<std::string> comments;

   unsigned long maxGPR = 0;
   unsigned long maxStack = 0;
   unsigned long maxPixelExport = 0;
   unsigned long maxParamExport = 0;
   unsigned long maxPosExport = 0;
};
