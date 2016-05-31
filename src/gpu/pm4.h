#pragma once
#include <gsl.h>
#include "pm4_format.h"
#include "latte_registers.h"
#include "utils/bitfield.h"
#include "utils/virtual_ptr.h"

#pragma pack(push, 1)

namespace pm4
{

struct DecafSwapBuffers
{
   static const auto Opcode = type3::DECAF_SWAP_BUFFERS;

   template<typename Serialiser>
   void serialise(Serialiser &se)
   {
   }
};

struct DecafSetContextState
{
   static const auto Opcode = type3::DECAF_SET_CONTEXT_STATE;

   virtual_ptr<void> context;

   template<typename Serialiser>
   void serialise(Serialiser &se)
   {
      se(context);
   }
};

struct DecafCopyColorToScan
{
   static const auto Opcode = type3::DECAF_COPY_COLOR_TO_SCAN;

   uint32_t scanTarget;
   uint32_t bufferAddr;
   uint32_t aaBufferAddr;
   latte::CB_COLORN_SIZE cb_color_size;
   latte::CB_COLORN_INFO cb_color_info;
   latte::CB_COLORN_VIEW cb_color_view;
   latte::CB_COLORN_MASK cb_color_mask;

   template<typename Serialiser>
   void serialise(Serialiser &se)
   {
      se(scanTarget);
      se(bufferAddr);
      se(aaBufferAddr);
      se(cb_color_size.value);
      se(cb_color_info.value);
      se(cb_color_view.value);
      se(cb_color_mask.value);
   }
};

struct DecafClearColor
{
   static const auto Opcode = type3::DECAF_CLEAR_COLOR;

   float red;
   float green;
   float blue;
   float alpha;
   uint32_t bufferAddr;
   uint32_t aaBufferAddr;
   latte::CB_COLORN_SIZE cb_color_size;
   latte::CB_COLORN_INFO cb_color_info;
   latte::CB_COLORN_VIEW cb_color_view;
   latte::CB_COLORN_MASK cb_color_mask;

   template<typename Serialiser>
   void serialise(Serialiser &se)
   {
      se(red);
      se(green);
      se(blue);
      se(alpha);
      se(bufferAddr);
      se(aaBufferAddr);
      se(cb_color_size.value);
      se(cb_color_info.value);
      se(cb_color_view.value);
      se(cb_color_mask.value);
   }
};

struct DecafClearDepthStencil
{
   static const auto Opcode = type3::DECAF_CLEAR_DEPTH_STENCIL;

   uint32_t flags;
   uint32_t bufferAddr;
   latte::DB_DEPTH_INFO db_depth_info;
   uint32_t hiZAddr;
   latte::DB_DEPTH_SIZE db_depth_size;
   latte::DB_DEPTH_VIEW db_depth_view;

   template<typename Serialiser>
   void serialise(Serialiser &se)
   {
      se(flags);
      se(bufferAddr);
      se(db_depth_info.value);
      se(hiZAddr);
      se(db_depth_size.value);
      se(db_depth_view.value);
   }
};

struct DrawIndexAuto
{
   static const auto Opcode = type3::DRAW_INDEX_AUTO;

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
   static const auto Opcode = type3::DRAW_INDEX_2;

   uint32_t maxIndices;                      // VGT_DMA_MAX_SIZE
   virtual_ptr<void> addr;                   // VGT_DMA_BASE
   uint32_t numIndices;                      // VGT_DMA_SIZE
   latte::VGT_DRAW_INITIATOR drawInitiator;  // VGT_DRAW_INITIATOR

   template<typename Serialiser>
   void serialise(Serialiser &se)
   {
      uint32_t unusedAddrHi = 0;

      se(maxIndices);
      se(addr);
      se(unusedAddrHi);
      se(numIndices);
      se(drawInitiator.value);
   }
};

struct IndexType
{
   static const auto Opcode = type3::INDEX_TYPE;

   latte::VGT_DMA_INDEX_TYPE type; // VGT_DMA_INDEX_TYPE

   template<typename Serialiser>
   void serialise(Serialiser &se)
   {
      se(type.value);
   }
};

struct NumInstances
{
   static const auto Opcode = type3::NUM_INSTANCES;

   uint32_t count; // VGT_DMA_NUM_INSTANCES

   template<typename Serialiser>
   void serialise(Serialiser &se)
   {
      se(count);
   }
};

struct SetAluConsts
{
   static const auto Opcode = type3::SET_ALU_CONST;

   latte::Register id;
   gsl::span<uint32_t> values;

   template<typename Serialiser>
   void serialise(Serialiser &se)
   {
      se.REG_OFFSET(id, latte::Register::AluConstRegisterBase);
      se(values);
   }
};

struct SetConfigReg
{
   static const auto Opcode = type3::SET_CONFIG_REG;

   latte::Register id;
   uint32_t value;

   template<typename Serialiser>
   void serialise(Serialiser &se)
   {
      se.REG_OFFSET(id, latte::Register::ConfigRegisterBase);
      se(value);
   }
};

struct SetConfigRegs
{
   static const auto Opcode = type3::SET_CONFIG_REG;

   latte::Register id;
   gsl::span<uint32_t> values;

   template<typename Serialiser>
   void serialise(Serialiser &se)
   {
      se.REG_OFFSET(id, latte::Register::ConfigRegisterBase);
      se(values);
   }
};

struct SetContextReg
{
   static const auto Opcode = type3::SET_CONTEXT_REG;

   latte::Register id;
   uint32_t value;

   template<typename Serialiser>
   void serialise(Serialiser &se)
   {
      se.REG_OFFSET(id, latte::Register::ContextRegisterBase);
      se(value);
   }
};

struct SetContextRegs
{
   static const auto Opcode = type3::SET_CONTEXT_REG;

   latte::Register id;
   gsl::span<uint32_t> values;

   template<typename Serialiser>
   void serialise(Serialiser &se)
   {
      se.REG_OFFSET(id, latte::Register::ContextRegisterBase);
      se(values);
   }
};

struct SetControlConstant
{
   static const auto Opcode = type3::SET_CTL_CONST;

   latte::Register id;
   uint32_t value;

   template<typename Serialiser>
   void serialise(Serialiser &se)
   {
      se.REG_OFFSET(id, latte::Register::ControlRegisterBase);
      se(value);
   }
};

struct SetControlConstants
{
   static const auto Opcode = type3::SET_CTL_CONST;

   latte::Register id;
   gsl::span<uint32_t> values;

   template<typename Serialiser>
   void serialise(Serialiser &se)
   {
      se.REG_OFFSET(id, latte::Register::ControlRegisterBase);
      se(values);
   }
};

struct SetLoopConst
{
   static const auto Opcode = type3::SET_LOOP_CONST;

   latte::Register id;
   uint32_t value;

   template<typename Serialiser>
   void serialise(Serialiser &se)
   {
      se.REG_OFFSET(id, latte::Register::LoopConstRegisterBase);
      se(value);
   }
};

struct SetLoopConsts
{
   static const auto Opcode = type3::SET_LOOP_CONST;

   latte::Register id;
   gsl::span<uint32_t> values;

   template<typename Serialiser>
   void serialise(Serialiser &se)
   {
      se.REG_OFFSET(id, latte::Register::LoopConstRegisterBase);
      se(values);
   }
};

struct SetSamplerAttrib
{
   static const auto Opcode = type3::SET_SAMPLER;

   uint32_t id;
   latte::SQ_TEX_SAMPLER_WORD0_N word0;
   latte::SQ_TEX_SAMPLER_WORD1_N word1;
   latte::SQ_TEX_SAMPLER_WORD2_N word2;

   template<typename Serialiser>
   void serialise(Serialiser &se)
   {
      se.CONST_OFFSET(id);
      se(word0.value);
      se(word1.value);
      se(word2.value);
   }
};

struct SetSamplers
{
   static const auto Opcode = type3::SET_SAMPLER;

   latte::Register id;
   gsl::span<uint32_t> values;

   template<typename Serialiser>
   void serialise(Serialiser &se)
   {
      se.REG_OFFSET(id, latte::Register::SamplerRegisterBase);
      se(values);
   }
};

struct SetVtxResource
{
   static const auto Opcode = type3::SET_RESOURCE;

   uint32_t id;
   virtual_ptr<const void> baseAddress;
   uint32_t size;
   latte::SQ_VTX_CONSTANT_WORD2_N word2;
   latte::SQ_VTX_CONSTANT_WORD3_N word3;
   latte::SQ_VTX_CONSTANT_WORD6_N word6;

   template<typename Serialiser>
   void serialise(Serialiser &se)
   {
      uint32_t unusedWord4 = 0xABCD1234;
      uint32_t unusedWord5 = 0xABCD1234;

      se.CONST_OFFSET(id);
      se(baseAddress);
      se(size);
      se(word2.value);
      se(word3.value);
      se(unusedWord4);
      se(unusedWord5);
      se(word6.value);
   }
};

struct SetTexResource
{
   static const auto Opcode = type3::SET_RESOURCE;

   uint32_t id;
   latte::SQ_TEX_RESOURCE_WORD0_N word0;
   latte::SQ_TEX_RESOURCE_WORD1_N word1;
   latte::SQ_TEX_RESOURCE_WORD2_N word2;
   latte::SQ_TEX_RESOURCE_WORD3_N word3;
   latte::SQ_TEX_RESOURCE_WORD4_N word4;
   latte::SQ_TEX_RESOURCE_WORD5_N word5;
   latte::SQ_TEX_RESOURCE_WORD6_N word6;

   template<typename Serialiser>
   void serialise(Serialiser &se)
   {
      se.CONST_OFFSET(id);
      se(word0.value);
      se(word1.value);
      se(word2.value);
      se(word3.value);
      se(word4.value);
      se(word5.value);
      se(word6.value);
   }
};

struct SetResources
{
   static const auto Opcode = type3::SET_RESOURCE;

   uint32_t id;
   gsl::span<uint32_t> values;

   template<typename Serialiser>
   void serialise(Serialiser &se)
   {
      se.CONST_OFFSET(id);
      se(values);
   }
};

struct IndirectBufferCall
{
   static const auto Opcode = type3::INDIRECT_BUFFER_PRIV;
   virtual_ptr<void> addr;
   uint32_t size;

   template<typename Serialiser>
   void serialise(Serialiser &se)
   {
      uint32_t unusedAddrHi = 0;

      se(addr);
      se(unusedAddrHi);
      se(size);
   }
};

struct LoadConfigReg
{
   static const auto Opcode = type3::LOAD_CONFIG_REG;
   virtual_ptr<uint32_t> addr;
   gsl::span<std::pair<uint32_t, uint32_t>> values;

   template<typename Serialiser>
   void serialise(Serialiser &se)
   {
      uint32_t unusedAddrHi = 0;

      se(addr);
      se(unusedAddrHi);
      se(values);
   }
};

struct LoadContextReg
{
   static const auto Opcode = type3::LOAD_CONTEXT_REG;
   virtual_ptr<uint32_t> addr;
   gsl::span<std::pair<uint32_t, uint32_t>> values;

   template<typename Serialiser>
   void serialise(Serialiser &se)
   {
      uint32_t unusedAddrHi = 0;

      se(addr);
      se(unusedAddrHi);
      se(values);
   }
};

struct LoadAluConst
{
   static const auto Opcode = type3::LOAD_ALU_CONST;
   virtual_ptr<uint32_t> addr;
   gsl::span<std::pair<uint32_t, uint32_t>> values;

   template<typename Serialiser>
   void serialise(Serialiser &se)
   {
      uint32_t unusedAddrHi = 0;

      se(addr);
      se(unusedAddrHi);
      se(values);
   }
};

struct LoadBoolConst
{
   static const auto Opcode = type3::LOAD_BOOL_CONST;
   virtual_ptr<uint32_t> addr;
   gsl::span<std::pair<uint32_t, uint32_t>> values;

   template<typename Serialiser>
   void serialise(Serialiser &se)
   {
      uint32_t unusedAddrHi = 0;

      se(addr);
      se(unusedAddrHi);
      se(values);
   }
};

struct LoadLoopConst
{
   static const auto Opcode = type3::LOAD_LOOP_CONST;
   virtual_ptr<uint32_t> addr;
   gsl::span<std::pair<uint32_t, uint32_t>> values;

   template<typename Serialiser>
   void serialise(Serialiser &se)
   {
      uint32_t unusedAddrHi = 0;

      se(addr);
      se(unusedAddrHi);
      se(values);
   }
};

struct LoadResource
{
   static const auto Opcode = type3::LOAD_RESOURCE;
   virtual_ptr<uint32_t> addr;
   gsl::span<std::pair<uint32_t, uint32_t>> values;

   template<typename Serialiser>
   void serialise(Serialiser &se)
   {
      uint32_t unusedAddrHi = 0;

      se(addr);
      se(unusedAddrHi);
      se(values);
   }
};

struct LoadSampler
{
   static const auto Opcode = type3::LOAD_SAMPLER;
   virtual_ptr<uint32_t> addr;
   gsl::span<std::pair<uint32_t, uint32_t>> values;

   template<typename Serialiser>
   void serialise(Serialiser &se)
   {
      uint32_t unusedAddrHi = 0;

      se(addr);
      se(unusedAddrHi);
      se(values);
   }
};

struct LoadControlConst
{
   static const auto Opcode = type3::LOAD_CTL_CONST;
   virtual_ptr<uint32_t> addr;
   gsl::span<std::pair<uint32_t, uint32_t>> values;

   template<typename Serialiser>
   void serialise(Serialiser &se)
   {
      uint32_t unusedAddrHi = 0;

      se(addr);
      se(unusedAddrHi);
      se(values);
   }
};

struct MW_ADDR_LO : Bitfield<MW_ADDR_LO, uint32_t>
{
   BITFIELD_ENTRY(0, 2, latte::CB_ENDIAN, ENDIAN_SWAP);
   BITFIELD_ENTRY(2, 30, uint32_t, ADDR_LO);
};

enum MW_CNTR_SEL : uint32_t
{
   MW_WRITE_DATA     = 0,
   MW_WRITE_CLOCK    = 1,
};

struct MW_ADDR_HI : Bitfield<MW_ADDR_HI, uint32_t>
{
   BITFIELD_ENTRY(0, 8, uint32_t, ADDR_HI);
   BITFIELD_ENTRY(16, 1, MW_CNTR_SEL, CNTR_SEL);
   BITFIELD_ENTRY(17, 1, bool, WR_CONFIRM);
   BITFIELD_ENTRY(18, 1, bool, DATA32);
};

struct MemWrite
{
   static const auto Opcode = type3::MEM_WRITE;
   MW_ADDR_LO addrLo;
   MW_ADDR_HI addrHi;
   uint32_t dataLo;
   uint32_t dataHi;

   template<typename Serialiser>
   void serialise(Serialiser &se)
   {
      se(addrLo.value);
      se(addrHi.value);
      se(dataLo);
      se(dataHi);
   }
};

struct EW_EOP_ADDR_LO : Bitfield<EW_EOP_ADDR_LO, uint32_t>
{
   BITFIELD_ENTRY(0, 2, latte::CB_ENDIAN, ENDIAN_SWAP);
   BITFIELD_ENTRY(2, 30, uint32_t, ADDR_LO);
};

enum EW_DATA_SEL : uint32_t
{
   EW_DATA_DISCARD      = 0,
   EW_DATA_32           = 1,
   EW_DATA_64           = 2,
   EW_DATA_CLOCK        = 3,
};

enum EW_INT_SEL : uint32_t
{
   EW_INT_NONE          = 0,
   EW_INT_ONLY          = 1,
   EW_INT_WRITE_CONFIRM = 2,
};

struct EW_EOP_ADDR_HI : Bitfield<EW_EOP_ADDR_HI, uint32_t>
{
   BITFIELD_ENTRY(0, 8, uint32_t, ADDR_HI);
   BITFIELD_ENTRY(24, 2, EW_INT_SEL, INT_SEL);
   BITFIELD_ENTRY(29, 3, EW_DATA_SEL, DATA_SEL);
};

struct EventWriteEOP
{
   static const auto Opcode = type3::EVENT_WRITE_EOP;
   latte::VGT_EVENT_INITIATOR eventInitiator;
   EW_EOP_ADDR_LO addrLo;
   EW_EOP_ADDR_HI addrHi;
   uint32_t dataLo;
   uint32_t dataHi;

   template<typename Serialiser>
   void serialise(Serialiser &se)
   {
      se(eventInitiator.value);
      se(addrLo.value);
      se(addrHi.value);
      se(dataLo);
      se(dataHi);
   }
};

}

#pragma pack(pop)
