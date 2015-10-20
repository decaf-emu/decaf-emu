#pragma once
#include "modules/vpad/vpad_status.h"
#include "types.h"
#include "utils/be_val.h"
#include "utils/be_vec.h"
#include "utils/structsize.h"

struct KPADExtStatus
{
   UNKNOWN(0x50);
};

struct KPADMotionPlusStatus
{
   UNKNOWN(0x40);
};

struct KPADStatus
{
   be_val<Buttons::Buttons> hold;
   be_val<Buttons::Buttons> trigger;
   be_val<Buttons::Buttons> release;
   UNKNOWN(0x48);
   be_vec2<float> accVertical;
   UNKNOWN(0x04);
   KPADExtStatus ext;
   KPADMotionPlusStatus motionPlus;
};
CHECK_OFFSET(KPADStatus, 0x00, hold);
CHECK_OFFSET(KPADStatus, 0x04, trigger);
CHECK_OFFSET(KPADStatus, 0x08, release);
CHECK_OFFSET(KPADStatus, 0x54, accVertical);
CHECK_OFFSET(KPADStatus, 0x60, ext);
CHECK_OFFSET(KPADStatus, 0xB0, motionPlus);
CHECK_SIZE(KPADStatus, 0xF0);

int32_t
KPADRead(uint32_t chan, KPADStatus *buffers, uint32_t count);

int32_t
KPADReadEx(uint32_t chan, KPADStatus *buffers, uint32_t count, be_val<int32_t> *error);
