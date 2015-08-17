#pragma once
#include "latte_opcodes.h"

namespace latte
{

namespace shadir
{

struct Instruction
{
   enum Type
   {
      ControlFlow,
      ALU,
      TEX
   };

   Instruction(Type type) :
      insType(type)
   {
   }

   Type insType;
   const char *name = nullptr;
   uint32_t cfPC = -1;
   uint32_t groupPC = -1;
};

struct CfInstruction : Instruction
{
   CfInstruction() :
      Instruction(Instruction::ControlFlow)
   {
   }

   latte::cf::inst id;
   uint32_t addr = 0;
   uint8_t popCount = 0;
   uint8_t loopCfConstant = 0;
   latte::cf::Cond::Cond cond = latte::cf::Cond::Active;
};

struct AluSource
{
   enum Type
   {
      Register,
      KcacheBank0,
      KcacheBank1,
      PreviousVector,
      PreviousScalar,
      ConstantFile,
      ConstantFloat,
      ConstantDouble,
      ConstantInt,
   };

   Type type;
   uint32_t id = 0;
   bool negate = false;
   bool absolute = false;
   latte::alu::Channel::Channel chan = latte::alu::Channel::Unknown;

   union
   {
      float floatValue;
      int intValue;
   };
};

struct AluDest
{
   uint32_t id = 0;
   latte::alu::Channel::Channel chan = latte::alu::Channel::Unknown;
   bool clamp = false;
};

struct AluInstruction : Instruction
{
   enum OpType
   {
      OP2,
      OP3
   };

   AluInstruction() :
      Instruction(Instruction::ALU)
   {
   }

   union
   {
      latte::alu::op2 op2;
      latte::alu::op3 op3;
   };

   OpType opType;
   uint32_t numSources = 0;
   AluSource sources[3];
   AluDest dest;
   latte::alu::PredicateSelect::PredicateSelect predSel = latte::alu::PredicateSelect::Off;
   latte::alu::OutputModifier::OutputModifier outputModifier = latte::alu::OutputModifier::Off;
   bool updateExecutionMask = false;
   bool updatePredicate = false;
   bool writeMask = false;
};

struct TexRegister
{
   uint8_t id = 0;
   latte::alu::Select::Select selX = latte::alu::Select::X;
   latte::alu::Select::Select selY = latte::alu::Select::Y;
   latte::alu::Select::Select selZ = latte::alu::Select::Z;
   latte::alu::Select::Select selW = latte::alu::Select::W;
};

struct TexInstruction : Instruction
{
   TexInstruction() :
      Instruction(Instruction::TEX)
   {
   }

   latte::tex::inst id;
   bool bcFracMode = false;
   bool fetchWholeQuad = false;
   uint8_t resourceID = 0;
   uint8_t samplerID = 0;
   int8_t lodBias = 0;
   TexRegister src;
   TexRegister dst;
   bool coordNormaliseX = false;
   bool coordNormaliseY = false;
   bool coordNormaliseZ = false;
   bool coordNormaliseW = false;
   int8_t offsetX = 0;
   int8_t offsetY = 0;
   int8_t offsetZ = 0;
};

struct Block
{
   enum Type
   {
      CodeBlock,
      Loop,
      Conditional
   };

   Block(Type type) :
      type(type)
   {
   }

   virtual ~Block()
   {
   }

   Type type;
};

struct CodeBlock : Block
{
   CodeBlock() :
      Block(Block::CodeBlock)
   {
   }

   std::vector<shadir::Instruction *> code; // Non-owning
};

struct LoopBlock : Block
{
   LoopBlock() :
      Block(Block::Loop)
   {
   }

   virtual ~LoopBlock() final
   {
      for (auto block : inner) {
         delete block;
      }
   }

   std::vector<Block *> inner;
};

struct ConditionalBlock : Block
{
   ConditionalBlock(shadir::Instruction *condition) :
      Block(Block::Conditional),
      condition(condition)
   {
   }

   virtual ~ConditionalBlock() final
   {
      for (auto block : inner) {
         delete block;
      }

      for (auto block : innerElse) {
         delete block;
      }
   }

   shadir::Instruction *condition; // Non-owning
   std::vector<Block *> inner;
   std::vector<Block *> innerElse;
};

} // namespace shadir

} // namespace latte

