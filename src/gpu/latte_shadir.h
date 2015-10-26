#pragma once
#include <memory>
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
      TEX,
      Export,
      AluReduction
   };

   Instruction(Type type) :
      insType(type)
   {
   }

   virtual ~Instruction()
   {
   }

   Type insType;
   const char *name = nullptr;
   int32_t cfPC = -1;
   int32_t groupPC = -1;
};

struct CfInstruction : Instruction
{
   CfInstruction() :
      Instruction(Instruction::ControlFlow)
   {
   }

   virtual ~CfInstruction() final
   {
   }

   latte::cf::inst id;
   uint32_t addr = 0;
   uint8_t popCount = 0;
   uint8_t loopCfConstant = 0;
   latte::cf::Cond::Cond cond = latte::cf::Cond::Active;
};

struct SelRegister
{
   uint8_t id = 0;
   latte::alu::Select::Select selX = latte::alu::Select::X;
   latte::alu::Select::Select selY = latte::alu::Select::Y;
   latte::alu::Select::Select selZ = latte::alu::Select::Z;
   latte::alu::Select::Select selW = latte::alu::Select::W;
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
      ConstantLiteral,
   };

   enum ValueType
   {
      Float,
      Int,
      Uint
   };

   Type type;
   ValueType valueType = ValueType::Float;
   uint32_t id = 0;
   bool negate = false;
   bool absolute = false;
   bool rel = false;
   latte::alu::IndexMode::IndexMode indexMode = latte::alu::IndexMode::ArX;
   latte::alu::Channel::Channel chan = latte::alu::Channel::Unknown;

   union
   {
      float floatValue;
      double doubleValue;
      int intValue;
      uint32_t literalValue;
   };
};

struct AluDest
{
   enum ValueType
   {
      Float,
      Int,
      Uint
   };

   uint32_t id = 0;
   latte::alu::Channel::Channel chan = latte::alu::Channel::Unknown;
   bool clamp = false;
   bool scalar = false;
   ValueType valueType = ValueType::Float;
};

struct AluInstruction : Instruction
{
   enum OpType
   {
      OP2,
      OP3
   };

   enum Unit
   {
      X,
      Y,
      Z,
      W,
      T
   };

   AluInstruction() :
      Instruction(Instruction::ALU)
   {
   }

   virtual ~AluInstruction() final
   {
   }

   union
   {
      latte::alu::op2 op2;
      latte::alu::op3 op3;
   };

   Unit unit;
   OpType opType;
   uint32_t numSources = 0;
   AluSource sources[3];
   AluDest dest;
   latte::alu::PredicateSelect::PredicateSelect predSel = latte::alu::PredicateSelect::Off;
   latte::alu::OutputModifier::OutputModifier outputModifier = latte::alu::OutputModifier::Off;
   bool updateExecutionMask = false;
   bool updatePredicate = false;
   bool writeMask = true;
   bool isReduction = false;
};

struct AluReductionInstruction : Instruction
{
   AluReductionInstruction() :
      Instruction(Instruction::AluReduction)
   {
   }

   virtual ~AluReductionInstruction() final
   {
   }

   std::unique_ptr<AluInstruction> units[4];
   latte::alu::op2 op2;
};

struct ExportInstruction : Instruction
{
   ExportInstruction() :
      Instruction(Instruction::Export)
   {
   }

   virtual ~ExportInstruction() final
   {
   }

   latte::exp::inst id;
   SelRegister src;
   uint32_t dstReg = 0;
   latte::exp::Type::Type type;
   uint32_t elemSize = 0;
   uint32_t indexGpr = 0;
   bool wholeQuadMode = false;
   bool barrier = false;
};

struct TexInstruction : Instruction
{
   TexInstruction() :
      Instruction(Instruction::TEX)
   {
   }

   virtual ~TexInstruction() final
   {
   }

   latte::tex::inst id;
   bool bcFracMode = false;
   bool fetchWholeQuad = false;
   uint8_t resourceID = 0;
   uint8_t samplerID = 0;
   int8_t lodBias = 0;
   SelRegister src;
   SelRegister dst;
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

using BlockList = std::vector<std::unique_ptr<shadir::Block>>;

struct CodeBlock : Block
{
   CodeBlock() :
      Block(Block::CodeBlock)
   {
   }

   virtual ~CodeBlock() final
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
   }

   BlockList inner;
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
   }

   shadir::Instruction *condition; // Non-owning
   BlockList inner;
   BlockList innerElse;
};

} // namespace shadir

} // namespace latte

