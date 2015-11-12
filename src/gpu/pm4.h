#pragma once
#include <array_view.h>
#include "pm4_format.h"
#include "latte_registers.h"
#include "utils/virtual_ptr.h"

#pragma pack(push, 1)

namespace pm4
{

struct DrawIndexAuto
{
   static const auto Opcode = Opcode3::DRAW_INDEX_AUTO;

   uint32_t indexCount;
   latte::VGT_DRAW_INITIATOR drawInitiator;

   template<typename Serialiser>
   void serialise(Serialiser &se)
   {
      se(indexCount);
      se(drawInitiator.value);
   }
};

struct DrawIndex2
{
   static const auto Opcode = Opcode3::DRAW_INDEX_2;

   uint32_t maxIndices;                      // VGT_DMA_MAX_SIZE
   virtual_ptr<void> addr;                   // VGT_DMA_BASE
   uint32_t numIndices;                      // VGT_DMA_SIZE
   latte::VGT_DRAW_INITIATOR drawInitiator;  // VGT_DRAW_INITIATOR

   template<typename Serialiser>
   void serialise(Serialiser &se)
   {
      se(maxIndices);
      se(addr);
      se(0);
      se(numIndices);
      se(drawInitiator.value);
   }
};

struct IndexType
{
   static const auto Opcode = Opcode3::INDEX_TYPE;

   latte::VGT_DMA_INDEX_TYPE type; // VGT_DMA_INDEX_TYPE

   template<typename Serialiser>
   void serialise(Serialiser &se)
   {
      se(type.value);
   }
};

struct NumInstances
{
   static const auto Opcode = Opcode3::NUM_INSTANCES;

   uint32_t count; // VGT_DMA_NUM_INSTANCES

   template<typename Serialiser>
   void serialise(Serialiser &se)
   {
      se(count);
   }
};

struct SetConfigReg
{
   static const auto Opcode = Opcode3::SET_CONFIG_REG;

   latte::Register::Value id;
   gsl::array_view<uint32_t> values;

   template<typename Serialiser>
   void serialise(Serialiser &se)
   {
      se.reg(id, latte::Register::ConfigRegisterBase);
      se(values);
   }
};

struct SetContextReg
{
   static const auto Opcode = Opcode3::SET_CONTEXT_REG;

   latte::Register::Value id;
   gsl::array_view<uint32_t> values;

   template<typename Serialiser>
   void serialise(Serialiser &se)
   {
      se.reg(id, latte::Register::ContextRegisterBase);
      se(values);
   }
};

struct SetControlConstant
{
   static const auto Opcode = Opcode3::SET_CTL_CONST;

   uint32_t id;
   gsl::array_view<uint32_t> values;

   template<typename Serialiser>
   void serialise(Serialiser &se)
   {
      se(id);
      se(values);
   }
};

struct SetResourceAttrib
{
   static const auto Opcode = Opcode3::SET_RESOURCE;

   uint32_t id;
   virtual_ptr<void> baseAddress;
   uint32_t size;
   latte::SQ_VTX_CONSTANT_WORD2_0 word2;
   latte::SQ_VTX_CONSTANT_WORD3_0 word3;
   latte::SQ_VTX_CONSTANT_WORD6_0 word6;

   template<typename Serialiser>
   void serialise(Serialiser &se)
   {
      se(id);
      se(baseAddress);
      se(size);
      se(word2.value);
      se(word3.value);
      se(0); // word4
      se(0); // word5
      se(word6.value);
      se(0); // word6
   }
};

/*
static inline void indexType(uint32_t indexType, uint32_t swapMode)
{
   uint32_t indexField;
   indexField |= (indexType & 1) << 0;
   indexField |= (swapMode & 3) << 1;

   auto pak = beginPacket<Opcode3::INDEX_TYPE>();
   pak.write(indexField);
}

static inline void drawIndex(uint32_t indexAddr, uint32_t numIndices)
{
   uint32_t drawInitiator = 0u;

   auto pak = beginPacket<Opcode3::DRAW_INDEX>();
   pak.write(indexAddr);
   pak.write(0u);
   pak.write(numIndices);
   pak.write(drawInitiator);
}

static inline void drawIndexAuto(uint32_t numVertices)
{
   uint32_t drawInitiator = 0u;

   auto pak = beginPacket<Opcode3::DRAW_INDEX_AUTO>();
   pak.write(numVertices);
   pak.write(drawInitiator);
}

static inline void drawIndexImmd(gsl::array_view<uint16_t> indices)
{
   auto numIndices = static_cast<uint32_t>(indices.size());
   uint32_t drawInitiator = 0u;

   auto pak = beginPacket<Opcode3::DRAW_INDEX_IMMD>();
   pak.write(numIndices);
   pak.write(drawInitiator);
   pak.writeArr(indices);
}

static inline void drawIndexImmd(gsl::array_view<uint32_t> indices)
{
   auto numIndices = static_cast<uint32_t>(indices.size());
   uint32_t drawInitiator = 0u;

   auto pak = beginPacket<Opcode3::DRAW_INDEX_IMMD>();
   pak.write(numIndices);
   pak.write(drawInitiator);
   pak.writeArr(indices);
}

static inline void numInstances(uint32_t count)
{
   auto pak = beginPacket<Opcode3::NUM_INSTANCES>();
   pak.write(count);
}

static inline void setConfigReg(ConfigRegister::Value id, gsl::array_view<uint32_t> values)
{
   auto pak = beginPacket<Opcode3::SET_CONFIG_REG>();
   pak.write(id - ConfigRegister::Base);
   pak.writeArr(values);
}

static inline void setContextReg(ContextRegister::Value id, gsl::array_view<uint32_t> values)
{
   auto pak = beginPacket<Opcode3::SET_CONTEXT_REG>();
   pak.write(id - ContextRegister::Base);
   pak.writeArr(values);
}

static inline void setAluConst(uint32_t id, gsl::array_view<uint32_t> values)
{
   auto pak = beginPacket<Opcode3::SET_ALU_CONST>();
   pak.write(id);
   pak.writeArr(values);
}

static inline void setBoolConst(uint32_t id, gsl::array_view<uint32_t> values)
{
   auto pak = beginPacket<Opcode3::SET_BOOL_CONST>();
   pak.write(id);
   pak.writeArr(values);
}

static inline void setLoopConst(uint32_t id, gsl::array_view<uint32_t> values)
{
   auto pak = beginPacket<Opcode3::SET_LOOP_CONST>();
   pak.write(id);
   pak.writeArr(values);
}

static inline void setResource(uint32_t id, gsl::array_view<uint32_t> values)
{
   auto pak = beginPacket<Opcode3::SET_RESOURCE>();
   pak.write(id);
   pak.writeArr(values);
}

static inline void setSampler(uint32_t id, gsl::array_view<uint32_t> values)
{
   auto pak = beginPacket<Opcode3::SET_SAMPLER>();
   pak.write(id);
   pak.writeArr(values);
}

static inline void setCtlConst(uint32_t id, gsl::array_view<uint32_t> values)
{
   auto pak = beginPacket<Opcode3::SET_CTL_CONST>();
   pak.write(id);
   pak.writeArr(values);
}*/

}

#pragma pack(pop)
