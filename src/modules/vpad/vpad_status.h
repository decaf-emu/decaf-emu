#pragma once
#include "types.h"
#include "utils/be_val.h"
#include "utils/be_vec.h"
#include "utils/structsize.h"

namespace Buttons
{
enum Buttons : uint32_t
{
   Sync     = 1 << 0,
   Home     = 1 << 1,
   Minus    = 1 << 2,
   Plus     = 1 << 3,
   R        = 1 << 4,
   L        = 1 << 5,
   ZR       = 1 << 6,
   ZL       = 1 << 7,
   Down     = 1 << 8,
   Up       = 1 << 9,
   Right    = 1 << 10,
   Left     = 1 << 11,
   Y        = 1 << 12,
   X        = 1 << 13,
   B        = 1 << 14,
   A        = 1 << 15,
   // ?
   StickR   = 1 << 17,
   StickL   = 1 << 18,
};
}

namespace TouchPadValidity
{
enum Validity : uint16_t
{
   Valid = 0,
   InvalidX = 1 << 0,
   InvalidY = 1 << 1,
   InvalidXY = InvalidX | InvalidY
};
}

namespace VpadReadError
{
enum Error : int32_t
{
   Success = 0,
   NoSamples = -1,
   InvalidController = -2,
};
}

struct VPADTouchPadStatus
{
   be_val<uint16_t> x;
   be_val<uint16_t> y;
   be_val<uint16_t> touched;
   be_val<TouchPadValidity::Validity> validity;
};
CHECK_OFFSET(VPADTouchPadStatus, 0x00, x);
CHECK_OFFSET(VPADTouchPadStatus, 0x02, y);
CHECK_OFFSET(VPADTouchPadStatus, 0x04, touched);
CHECK_OFFSET(VPADTouchPadStatus, 0x06, validity);
CHECK_SIZE(VPADTouchPadStatus, 8);

struct VPADAccStatus
{
   be_val<float> unk1;
   be_val<float> unk2;
   be_val<float> unk3;
   be_val<float> unk4;
   be_val<float> unk5;
   be_vec2<float> vertical;
};
CHECK_OFFSET(VPADAccStatus, 0x14, vertical);
CHECK_SIZE(VPADAccStatus, 0x1c);

struct VPADGyroStatus
{
   be_val<float> unk1;
   be_val<float> unk2;
   be_val<float> unk3;
   be_val<float> unk4;
   be_val<float> unk5;
   be_val<float> unk6;
};
CHECK_SIZE(VPADGyroStatus, 0x18);

struct VPADStatus
{
   be_val<Buttons::Buttons> hold;
   be_val<Buttons::Buttons> trigger;
   be_val<Buttons::Buttons> release;
   be_vec2<float> leftStick;
   be_vec2<float> rightStick;
   VPADAccStatus accelorometer;
   VPADGyroStatus gyro;
   UNKNOWN(0x02);
   VPADTouchPadStatus tpNormal;
   VPADTouchPadStatus tpFiltered1;
   VPADTouchPadStatus tpFiltered2;
   UNKNOWN(0x28);
   be_vec3<float> mag;
   be_val<uint8_t> slideVolume;
   be_val<uint8_t> battery;
   be_val<uint8_t> micStatus;
   be_val<uint8_t> slideVolumeEx;
   UNKNOWN(0x07);
};
CHECK_OFFSET(VPADStatus, 0x00, hold);
CHECK_OFFSET(VPADStatus, 0x04, trigger);
CHECK_OFFSET(VPADStatus, 0x08, release);
CHECK_OFFSET(VPADStatus, 0x0C, leftStick);
CHECK_OFFSET(VPADStatus, 0x14, rightStick);
CHECK_OFFSET(VPADStatus, 0x1C, accelorometer);
CHECK_OFFSET(VPADStatus, 0x38, gyro);
CHECK_OFFSET(VPADStatus, 0x52, tpNormal);
CHECK_OFFSET(VPADStatus, 0x5A, tpFiltered1);
CHECK_OFFSET(VPADStatus, 0x62, tpFiltered2);
CHECK_OFFSET(VPADStatus, 0x94, mag);
CHECK_OFFSET(VPADStatus, 0xA0, slideVolume);
CHECK_OFFSET(VPADStatus, 0xA1, battery);
CHECK_OFFSET(VPADStatus, 0xA2, micStatus);
CHECK_OFFSET(VPADStatus, 0xA3, slideVolumeEx);
CHECK_SIZE(VPADStatus, 0xAC);

int32_t
VPADRead(uint32_t chan, VPADStatus *buffers, uint32_t count, be_val<VpadReadError::Error> *error);
