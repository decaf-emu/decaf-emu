#pragma once
#include "common/types.h"
#include "common/be_val.h"
#include "common/be_vec.h"
#include "common/structsize.h"
#include "vpad_enum.h"

namespace vpad
{

/**
 * \defgroup vpad_status VPAD Controller Status
 * \ingroup vpad
 * @{
 */

struct VPADTouchData
{
   be_val<uint16_t> x;
   be_val<uint16_t> y;
   be_val<uint16_t> touched;
   be_val<TouchPadValidity> validity;
};
CHECK_OFFSET(VPADTouchData, 0x00, x);
CHECK_OFFSET(VPADTouchData, 0x02, y);
CHECK_OFFSET(VPADTouchData, 0x04, touched);
CHECK_OFFSET(VPADTouchData, 0x06, validity);
CHECK_SIZE(VPADTouchData, 0x08);

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
   //! Indicates what buttons are held down
   be_val<Buttons> hold;

   //! Indicates what buttons have been pressed since last sample
   be_val<Buttons> trigger;

   //! Indicates what buttons have been released since last sample
   be_val<Buttons> release;

   //! Position of left analog stick
   be_vec2<float> leftStick;

   //! Position of right analog stick
   be_vec2<float> rightStick;

   //! Status of DRC accelorometer
   VPADAccStatus accelorometer;

   //! Status of DRC gyro
   VPADGyroStatus gyro;

   UNKNOWN(0x02);

   //! Current touch position on DRC
   VPADTouchData tpNormal;

   //! Filtered touch position, first level of smoothing
   VPADTouchData tpFiltered1;

   //! Filtered touch position, second level of smoothing
   VPADTouchData tpFiltered2;

   UNKNOWN(0x28);

   //! Status of DRC magnetometer
   be_vec3<float> mag;

   //! Current volume set by the slide control
   be_val<uint8_t> slideVolume;

   //! Battery level of controller
   be_val<uint8_t> battery;

   //! Status of DRC microphone
   be_val<uint8_t> micStatus;

   //! Unknown volume related value
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
VPADRead(uint32_t chan,
         VPADStatus *buffers,
         uint32_t count,
         be_val<VPADReadError> *error);

void
VPADGetTPCalibratedPoint(uint32_t chan,
                         VPADTouchData *calibratedData,
                         VPADTouchData *uncalibratedData);

/** @} */

} // namespace vpad
