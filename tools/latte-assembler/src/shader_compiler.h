#pragma once
#include <libgfd/gfd.h>
#include <libgpu/latte/latte_instructions.h>
#include <peglib.h>
#include <stdexcept>
#include <string>
#include <vector>

class parse_exception : public std::runtime_error
{
public:
   parse_exception(const std::string &m) :
      std::runtime_error(m)
   {
   }

private:
   std::string mMessage;
};

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
   gfd::GFDPixelShader pixelShader;
   gfd::GFDVertexShader vertexShader;
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

struct CommentKeyValue
{
   bool isValue() const
   {
      return member.empty() && index.empty();
   }

   bool isObject() const
   {
      return !member.empty() && index.empty();
   }

   bool isArrayOfValues() const
   {
      return !index.empty() && member.empty();
   }

   bool isArrayOfObjects() const
   {
      return !member.empty() && !index.empty();
   }

   std::string obj;
   std::string index;
   std::string member;
   std::string value;
};

// compiler_parse
bool
compileShaderCode(Shader &shader,
                  std::vector<char> &code);

bool
compileAST(Shader &shader,
           std::shared_ptr<peg::Ast> ast);

// compiler_alu
bool
compileAluClause(Shader &shader,
                 peg::Ast &node);

// compiler_exp
bool
compileExpInst(Shader &shader,
               peg::Ast &node);

// compiler_latte
latte::SQ_ALU_VEC_BANK_SWIZZLE
parseAluBankSwizzle(peg::Ast &node);

latte::SQ_INDEX_MODE
parseAluDstRelIndexMode(peg::Ast &node);

latte::SQ_CHAN
parseChan(peg::Ast &node);

latte::SQ_ALU_EXECUTE_MASK_OP
parseExecuteMaskOp(peg::Ast &node);

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
bool
parseComment(const std::string &comment,
             CommentKeyValue &out);

float
parseFloat(peg::Ast &node);

uint32_t
parseHexNumber(peg::Ast &node);

unsigned long
parseNumber(peg::Ast &node);

LiteralValue
parseLiteral(peg::Ast &node);

bool
parseValueBool(const std::string &value);

uint32_t
parseValueNumber(const std::string &value);

// compiler_gfd
bool
gfdAddVertexShader(gfd::GFDFile &file,
                   Shader &shader);

bool
gfdAddPixelShader(gfd::GFDFile &file,
                  Shader &shader);

void
ensureArrayOfObjects(const CommentKeyValue &kv);

void
ensureArrayOfValues(const CommentKeyValue &kv);

void
ensureObject(const CommentKeyValue &kv);

void
ensureValue(const CommentKeyValue &kv);

gx2::GX2ShaderVarType
parseShaderVarType(const std::string &v);

gx2::GX2ShaderMode
parseShaderMode(const std::string &v);

// compiler_gfd_vsh
bool
parseShaderComments(gfd::GFDVertexShader &shader,
                    std::vector<std::string> &comments);

// compiler_gfd_psh
bool
parseShaderComments(gfd::GFDPixelShader &shader,
                    std::vector<std::string> &comments);
