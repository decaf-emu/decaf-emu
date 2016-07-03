#pragma once
#include "common/types.h"
#include "common/be_val.h"
#include "common/be_vec.h"
#include "common/structsize.h"
#include "vpad_enum.h"

namespace vpad
{

struct VPADTouchPadStatus
{
   be_val<uint16_t> x;
   be_val<uint16_t> y;
   be_val<uint16_t> touched;
   be_val<TouchPadValidity> validity;
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
   be_val<Buttons> hold;
   be_val<Buttons> trigger;
   be_val<Buttons> release;
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

struct VPADTouchData
{
   be_val<uint16_t> x;
   be_val<uint16_t> y;
   be_val<uint16_t> down;
   be_val<uint16_t> unk1;
};
CHECK_OFFSET(VPADTouchData, 0x00, x);
CHECK_OFFSET(VPADTouchData, 0x02, y);
CHECK_OFFSET(VPADTouchData, 0x04, down);
CHECK_OFFSET(VPADTouchData, 0x06, unk1);
CHECK_SIZE(VPADTouchData, 0x8);

int32_t
VPADRead(uint32_t chan,
         VPADStatus *buffers,
         uint32_t count,
         be_val<VPADReadError> *error);

void
VPADGetTPCalibratedPoint(uint32_t chan,
                         VPADTouchData *calibratedData,
                         VPADTouchData *uncalibratedData);

} // namespace vpad
