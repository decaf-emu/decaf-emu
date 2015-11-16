#pragma once
#include "types.h"

#pragma pack(push, 1)

namespace pm4
{

namespace Opcode3
{
enum Value : uint32_t
{
   DECAF_COPY_COLOR_TO_SCAN   = 0x01,
   DECAF_SWAP_BUFFERS         = 0x02,

   NOP                        = 0x10,
   INDIRECT_BUFFER_END        = 0x17,
   NEXTCHAR                   = 0x19,
   PLY_NEXTSCAN               = 0x1D,
   SET_SCISSORS               = 0x1E,
   OCCLUSION_QUERY            = 0x1F,
   SET_PREDICATION            = 0x20,
   REG_RMW                    = 0x21,
   COND_EXEC                  = 0x22,
   PRED_EXEC                  = 0x23,
   START_3D_CMDBUF            = 0x24,
   START_2D_CMDBUF            = 0x25,
   INDEX_BASE                 = 0x26,
   DRAW_INDEX_2               = 0x27,
   CONTEXT_CONTROL            = 0x28,
   DRAW_INDEX_OFFSET          = 0x29,
   INDEX_TYPE                 = 0x2A,
   DRAW_INDEX                 = 0x2B,
   LOAD_PALETTE               = 0x2C,
   DRAW_INDEX_AUTO            = 0x2D,
   DRAW_INDEX_IMMD            = 0x2E,
   NUM_INSTANCES              = 0x2F,
   DRAW_INDEX_MULTI_AUTO      = 0x30,
   INDIRECT_BUFFER_PRIV       = 0x32,
   STRMOUT_BUFFER_UPDATE      = 0x34,
   DRAW_INDEX_OFFSET_2        = 0x35,
   DRAW_INDEX_MULTI_ELEMENT   = 0x36,
   INDIRECT_BUFFER_MP         = 0x38,
   MEM_SEMAPHORE              = 0x39,
   MPEG_INDEX                 = 0x3A,
   COPY_DW                    = 0x3B,
   WAIT_REG_MEM               = 0x3C,
   MEM_WRITE                  = 0x3D,
   PER_FRAME                  = 0x3E,
   INDIRECT_BUFFER            = 0x3F,
   CP_DMA                     = 0x41,
   PFP_SYNC_ME                = 0x42,
   SURFACE_SYNC               = 0x43,
   ME_INITIALIZE              = 0x44,
   COND_WRITE                 = 0x45,
   EVENT_WRITE                = 0x46,
   EVENT_WRITE_EOP            = 0x47,
   LOAD_SURFACE_PROBE         = 0x48,
   SURFACE_PROBE              = 0x49,
   PREAMBLE_CNTL              = 0x4A,
   RB_OFFSET                  = 0x4B,
   GFX_CNTX_UPDATE            = 0x52,
   BLK_CNTX_UPDATE            = 0x53,
   IB_OFFSET                  = 0x54,
   INCR_UPDT_STATE            = 0x55,
   INCR_UPDT_CONST            = 0x56,
   ONE_REG_WRITE              = 0x57,
   LOAD_CONFIG_REG            = 0x60,
   LOAD_CONTEXT_REG           = 0x61,
   LOAD_ALU_CONST             = 0x62,
   LOAD_BOOL_CONST            = 0x63,
   LOAD_LOOP_CONST            = 0x64,
   LOAD_RESOURCE              = 0x65,
   LOAD_SAMPLER               = 0x66,
   LOAD_CTL_CONST             = 0x67,
   SET_CONFIG_REG             = 0x68,
   SET_CONTEXT_REG            = 0x69,
   SET_ALU_CONST              = 0x6A,
   SET_BOOL_CONST             = 0x6B,
   SET_LOOP_CONST             = 0x6C,
   SET_RESOURCE               = 0x6D,
   SET_SAMPLER                = 0x6E,
   SET_CTL_CONST              = 0x6F,
   SET_RESOURCE_OFFSET        = 0x70,
   STRMOUT_BASE_UPDATE        = 0x72,
   SURFACE_BASE_UPDATE        = 0x73,
   SET_ALL_CONTEXTS           = 0x74,
   INDIRECT_BUFFER_BASE       = 0x78,
   EXECUTE_IB2                = 0x79,
   PFP_REG_WR                 = 0x7B,
   FORWARD_HEADER             = 0x7C,
   PAINT                      = 0x91,
   BITBLT                     = 0x92,
   HOSTDATA_BLT               = 0x94,
   POLYLINE                   = 0x95,
   POLYSCANLINES              = 0x98,
   PAINT_MULTI                = 0x9A,
   BITBLT_MULTI               = 0x9B,
   TRANS_BITBLT               = 0x9C,
   DRAW_2D_DIRTY_AREA         = 0xFF,
};
}

namespace PacketType
{
enum Value : uint32_t
{
   Type0 = 0,
   Type1 = 1,
   Type2 = 2,
   Type3 = 3
};
}

union PacketHeader
{
   uint32_t value;

   struct
   {
      uint32_t : 30;
      PacketType::Value type : 2;
   };
};

union Packet0
{
   uint32_t value;

   struct
   {
      uint32_t baseIndex : 16;
      uint32_t count : 14;
      PacketType::Value type : 2;
   };
};

union Packet2
{
   uint32_t value;

   struct
   {
      uint32_t : 30;
      PacketType::Value type : 2;
   };
};

union Packet3
{
   uint32_t value;

   struct
   {
      uint32_t predicate : 1;
      uint32_t : 7;
      Opcode3::Value opcode : 8;
      uint32_t size : 14;
      PacketType::Value type : 2;
   };
};

#pragma pack(pop)

} // namespace pm4
