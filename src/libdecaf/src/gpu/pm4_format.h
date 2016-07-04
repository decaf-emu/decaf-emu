#pragma once
#include "common/types.h"
#include "common/bitfield.h"

#pragma pack(push, 1)

namespace pm4
{

namespace type3
{
enum IT_OPCODE : uint32_t
{
   DECAF_COPY_COLOR_TO_SCAN   = 0x01,
   DECAF_SWAP_BUFFERS         = 0x02,
   DECAF_CLEAR_COLOR          = 0x03,
   DECAF_CLEAR_DEPTH_STENCIL  = 0x04,
   DECAF_SET_CONTEXT_STATE    = 0x05,
   DECAF_SET_BUFFER           = 0x06,
   DECAF_INVALIDATE           = 0x07,
   DECAF_DEBUGMARKER          = 0x08,

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
   CONTEXT_CTL                = 0x28,
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

BITFIELD(Header, uint32_t)
   enum Type
   {
      Type0 = 0,
      Type1 = 1,
      Type2 = 2,
      Type3 = 3,
   };

   BITFIELD_ENTRY(30, 8, Type, type);
BITFIELD_END

namespace type0
{

BITFIELD(Header, uint32_t)
   BITFIELD_ENTRY(0, 16, uint32_t, baseIndex);
   BITFIELD_ENTRY(16, 14, uint32_t, count);
   BITFIELD_ENTRY(30, 8, pm4::Header::Type, type);
BITFIELD_END

} // type0

namespace type2
{

BITFIELD(Header, uint32_t)
   BITFIELD_ENTRY(30, 8, pm4::Header::Type, type);
BITFIELD_END

} // type2

namespace type3
{

BITFIELD(Header, uint32_t)
   BITFIELD_ENTRY(0, 1, bool, predicate);
   BITFIELD_ENTRY(8, 8, IT_OPCODE, opcode);
   BITFIELD_ENTRY(16, 14, uint32_t, size);
   BITFIELD_ENTRY(30, 8, pm4::Header::Type, type);
BITFIELD_END

} // type3

#pragma pack(pop)

} // namespace pm4
