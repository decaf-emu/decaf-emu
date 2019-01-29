#ifndef LATTE_ENUM_CP_H
#define LATTE_ENUM_CP_H

#include <common/enum_start.inl>

ENUM_NAMESPACE_ENTER(latte)

// Interrupt types taken from linux/drivers/gpu/drm/radeon and
// avm.rpl dc.rpl tcl.rpl uvd.rpl
ENUM_BEG(CP_INT_SRC_ID, uint32_t)
   ENUM_VALUE(D1_VSYNC,                         1)
   ENUM_VALUE(D1_TRIGA,                         2)
   ENUM_VALUE(D2_VSYNC,                         5)
   ENUM_VALUE(D2_TRIGA,                         6)
   ENUM_VALUE(D1_VUPD,                          8)
   ENUM_VALUE(D1_PFLIP,                         9)
   ENUM_VALUE(D2_VUPD,                          10)
   ENUM_VALUE(D2_PFLIP,                         11)
   ENUM_VALUE(HPD_DAC_HOTPLUG,                  19)
   ENUM_VALUE(HDMI,                             21)
   ENUM_VALUE(DVOCAP,                           22)
   ENUM_VALUE(UVD,                              124)
   ENUM_VALUE(CP_RB,                            176)
   ENUM_VALUE(CP_IB1,                           177)
   ENUM_VALUE(CP_IB2,                           178)
   ENUM_VALUE(CP_RESERVED_BITS,                 180)
   ENUM_VALUE(CP_EOP_EVENT,                     181)
   ENUM_VALUE(SCRATCH,                          182)
   ENUM_VALUE(CP_BAD_OPCODE,                    183)
   ENUM_VALUE(CP_CTX_EMPTY,                     187)
   ENUM_VALUE(CP_CTX_BUSY,                      188)
   ENUM_VALUE(UNKNOWN_192,                      192)
   ENUM_VALUE(DMA_TRAP_EVENT,                   224)
   ENUM_VALUE(DMA_SEM_INCOMPLETE,               225)
   ENUM_VALUE(DMA_SEM_WAIT,                     226)
   ENUM_VALUE(THERMAL_LOW_TO_HIGH,              230)
   ENUM_VALUE(THERMAL_HIGH_TO_LOW,              231)
   ENUM_VALUE(GUI_IDLE,                         233)
   ENUM_VALUE(DMA_CTX_EMPTY,                    243)
ENUM_END(CP_INT_SRC_ID)

ENUM_NAMESPACE_EXIT(latte)

#include <common/enum_end.inl>

#endif // ifdef LATTE_ENUM_CP_H
