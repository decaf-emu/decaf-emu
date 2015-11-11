#pragma once
#include <fail_fast.h>
#include <initializer_list>
#include <array_view.h>
#include <vector>
#include "modules/coreinit/coreinit_time.h"
#include "types.h"
#include "utils/be_val.h"
#include "utils/virtual_ptr.h"

#pragma pack(push, 1)

namespace pm4
{

namespace Opcode3
{
enum Value : uint32_t
{
   NOP = 0x10,
   INDIRECT_BUFFER_END = 0x17,
   NEXTCHAR = 0x19,
   PLY_NEXTSCAN = 0x1D,
   SET_SCISSORS = 0x1E,
   OCCLUSION_QUERY = 0x1F,
   SET_PREDICATION = 0x20,
   REG_RMW = 0x21,
   COND_EXEC = 0x22,
   PRED_EXEC = 0x23,
   START_3D_CMDBUF = 0x24,
   START_2D_CMDBUF = 0x25,
   INDEX_BASE = 0x26,
   DRAW_INDEX_2 = 0x27,
   CONTEXT_CONTROL = 0x28,
   DRAW_INDEX_OFFSET = 0x29,
   INDEX_TYPE = 0x2A,
   DRAW_INDEX = 0x2B,
   LOAD_PALETTE = 0x2C,
   DRAW_INDEX_AUTO = 0x2D,
   DRAW_INDEX_IMMD = 0x2E,
   NUM_INSTANCES = 0x2F,
   DRAW_INDEX_MULTI_AUTO = 0x30,
   INDIRECT_BUFFER_PRIV = 0x32,
   STRMOUT_BUFFER_UPDATE = 0x34,
   DRAW_INDEX_OFFSET_2 = 0x35,
   DRAW_INDEX_MULTI_ELEMENT = 0x36,
   INDIRECT_BUFFER_MP = 0x38,
   MEM_SEMAPHORE = 0x39,
   MPEG_INDEX = 0x3A,
   COPY_DW = 0x3B,
   WAIT_REG_MEM = 0x3C,
   MEM_WRITE = 0x3D,
   PER_FRAME = 0x3E,
   INDIRECT_BUFFER = 0x3F,
   CP_DMA = 0x41,
   PFP_SYNC_ME = 0x42,
   SURFACE_SYNC = 0x43,
   ME_INITIALIZE = 0x44,
   COND_WRITE = 0x45,
   EVENT_WRITE = 0x46,
   EVENT_WRITE_EOP = 0x47,
   LOAD_SURFACE_PROBE = 0x48,
   SURFACE_PROBE = 0x49,
   PREAMBLE_CNTL = 0x4A,
   RB_OFFSET = 0x4B,
   GFX_CNTX_UPDATE = 0x52,
   BLK_CNTX_UPDATE = 0x53,
   IB_OFFSET = 0x54,
   INCR_UPDT_STATE = 0x55,
   INCR_UPDT_CONST = 0x56,
   ONE_REG_WRITE = 0x57,
   LOAD_CONFIG_REG = 0x60,
   LOAD_CONTEXT_REG = 0x61,
   LOAD_ALU_CONST = 0x62,
   LOAD_BOOL_CONST = 0x63,
   LOAD_LOOP_CONST = 0x64,
   LOAD_RESOURCE = 0x65,
   LOAD_SAMPLER = 0x66,
   LOAD_CTL_CONST = 0x67,
   SET_CONFIG_REG = 0x68,
   SET_CONTEXT_REG = 0x69,
   SET_ALU_CONST = 0x6A,
   SET_BOOL_CONST = 0x6B,
   SET_LOOP_CONST = 0x6C,
   SET_RESOURCE = 0x6D,
   SET_SAMPLER = 0x6E,
   SET_CTL_CONST = 0x6F,
   SET_RESOURCE_OFFSET = 0x70,
   STRMOUT_BASE_UPDATE = 0x72,
   SURFACE_BASE_UPDATE = 0x73,
   SET_ALL_CONTEXTS = 0x74,
   INDIRECT_BUFFER_BASE = 0x78,
   EXECUTE_IB2 = 0x79,
   PFP_REG_WR = 0x7B,
   FORWARD_HEADER = 0x7C,
   PAINT = 0x91,
   BITBLT = 0x92,
   HOSTDATA_BLT = 0x94,
   POLYLINE = 0x95,
   POLYSCANLINES = 0x98,
   PAINT_MULTI = 0x9A,
   BITBLT_MULTI = 0x9B,
   TRANS_BITBLT = 0x9C,
   DRAW_2D_DIRTY_AREA = 0xFF,
};
}

union PacketHeader
{
   uint32_t value;

   struct
   {
      uint32_t type : 2;
      uint32_t : 30;
   };
};

union Packet0
{
   uint32_t value;

   struct
   {
      uint32_t type : 2;
      uint32_t count : 14;
      uint32_t baseIndex : 16;
   };
};

union Packet2
{
   uint32_t value;

   struct
   {
      uint32_t type : 2;
      uint32_t : 30;
   };
};

union Packet3
{
   uint32_t value;

   struct
   {
      uint32_t type : 2;
      uint32_t size : 14;
      Opcode3::Value opcode : 8;
      uint32_t : 7;
      uint32_t predicate : 1;
   };
};

struct CommandBuffer
{
   OSTime submitTime;
   virtual_ptr<uint32_t> buffer;
   uint32_t curSize;
   uint32_t maxSize;
};

namespace ConfigRegister
{
enum Value : uint32_t
{
   Base = 0x2000,
};
}

namespace ContextRegister
{
enum Value : uint32_t
{
   Base = 0xA000,
   Blend0Control = 0xA1E0,
   Blend1Control = 0xA1E1,
   Blend2Control = 0xA1E2,
   Blend3Control = 0xA1E3,
   Blend4Control = 0xA1E4,
   Blend5Control = 0xA1E5,
   Blend6Control = 0xA1E6,
   Blend7Control = 0xA1E7,
};
}

CommandBuffer *getCommandBuffer(uint32_t size);
CommandBuffer *flushCommandBuffer(CommandBuffer *buffer);

class Pm4Builder {
public:
   Pm4Builder(Opcode3::Value opCode)
   {
      mBuffer = getCommandBuffer(0);
      mBufferStartSize = mBuffer->curSize;

      checkSpace(1);
      mHeader = reinterpret_cast<Packet3*>(&mBuffer->buffer[mBuffer->curSize++]);
      mHeader->type = 3;
      mHeader->opcode = opCode;
      mHeader->size = 0;
   }

   void write(uint32_t value) {
      checkSpace(1);
      mHeader->size++;
      mBuffer->buffer[mBuffer->curSize++] = value;
   }

   void writeArr(gsl::array_view<uint32_t> values)
   {
      checkSpace(static_cast<uint32_t>(values.size()));
      mHeader->size += static_cast<uint32_t>(values.size());
      for (auto &value : values) {
         mBuffer->buffer[mBuffer->curSize++] = value;
      }
   }

   void writeArr(gsl::array_view<uint16_t> values)
   {
      uint32_t dwordCount = (static_cast<uint32_t>(values.size()) + 1) / 2;
      checkSpace(dwordCount);
      mHeader->size += dwordCount;
      for (auto i = 0u; i < values.size() / 2; ++i) {
         uint32_t data = 0;
         data |= values[i * 2 + 0];
         data |= values[i * 2 + 1] << 16;
         mBuffer->buffer[mBuffer->curSize++] = data;
      }
      // if uneven number of 16bit values, write the last one
      if (values.size() & 1) {
         mBuffer->buffer[mBuffer->curSize++] = values[values.size() - 1];
      }
   }

protected:
   void checkSpace(uint32_t size) {
      if (mBuffer->curSize + size >= mBuffer->maxSize) {
         // Save our old buffer and fetch a ne wone
         CommandBuffer *oldBuffer = mBuffer;
         mBuffer = flushCommandBuffer(mBuffer);

         // If there is currently a packet being built, we need to copy it
         if (mBuffer->curSize > mBufferStartSize) {
            auto packetSize = mBuffer->curSize - mBufferStartSize;

            // Copy the packet data to the new buffer
            memcpy(mBuffer->buffer, &oldBuffer->buffer[mBufferStartSize], packetSize * sizeof(uint32_t));
            oldBuffer->curSize = mBufferStartSize;
            mBuffer->curSize += packetSize;
         }

         // Update the buffer start size
         mBufferStartSize = mBuffer->curSize;

         if (mBuffer->curSize + size >= mBuffer->maxSize) {
            // Still not enough room somehow...
            gsl::fail_fast();
         }
      }

      mBuffer->curSize += size;
   }

   CommandBuffer *mBuffer;
   uint32_t mBufferStartSize;
   Packet3 *mHeader;
};

template <Opcode3::Value opCode>
static inline Pm4Builder beginPacket()
{
   return Pm4Builder(opCode);
}

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
}

}

#pragma pack(pop)
