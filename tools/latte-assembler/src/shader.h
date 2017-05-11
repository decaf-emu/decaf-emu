#pragma once
#include <array>
#include <cstdint>
#include <libgpu/latte/latte_instructions.h>
#include <peglib.h>
#include <vector>
#include <string>

enum class ShaderType
{
   Invalid,
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
   uint32_t hexValue = 0;
   float floatValue = 0.0f;
};

struct AluGroup
{
   uint32_t clausePC = 0;
   std::vector<latte::AluInst> insts;
   std::vector<LiteralValue> literals;
};

struct AluClause
{
   uint32_t cfPC = 0;
   std::shared_ptr<peg::Ast> addrNode;
   std::shared_ptr<peg::Ast> countNode;
   std::vector<AluGroup> groups;
};

struct TexClause
{
   uint32_t cfPC = 0;
   uint32_t clausePC = 0;
   std::shared_ptr<peg::Ast> addrNode;
   std::shared_ptr<peg::Ast> countNode;
   std::vector<latte::TextureFetchInst> insts;
};

struct Shader
{
   Shader()
   {
      gprRead.fill(false);
      gprWritten.fill(false);
   }

   ShaderType type = ShaderType::Invalid;
   std::string path;

   uint32_t clausePC = 0;
   std::vector<latte::ControlFlowInst> cfInsts;

   std::vector<AluClause> aluClauses;
   uint32_t aluClauseBaseAddress;
   std::vector<uint32_t> aluClauseData;

   std::vector<TexClause> texClauses;
   uint32_t texClauseBaseAddress;
   std::vector<uint32_t> texClauseData;

   std::vector<std::string> comments;

   bool uniformBlocksUsed = false;
   bool uniformRegistersUsed = false;

   std::array<bool, 128> gprRead;
   std::array<bool, 128> gprWritten;

   unsigned long maxGPR = 0;
   unsigned long maxStack = 0;
   unsigned long maxPixelExport = 0;
   unsigned long maxParamExport = 0;
   unsigned long maxPosExport = 0;
};
