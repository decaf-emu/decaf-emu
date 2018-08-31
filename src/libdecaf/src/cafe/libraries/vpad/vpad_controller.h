#pragma once
#include "vpad_enum.h"
#include <libcpu/be2_struct.h>

namespace cafe::vpad
{

/**
 * \defgroup vpad_status VPAD Controller Status
 * \ingroup vpad
 * @{
 */

struct VPADVec2D
{
   be2_val<float> x;
   be2_val<float> y;
};
CHECK_OFFSET(VPADVec2D, 0x00, x);
CHECK_OFFSET(VPADVec2D, 0x04, y);
CHECK_SIZE(VPADVec2D, 0x08);

struct VPADVec3D
{
   be2_val<float> x;
   be2_val<float> y;
   be2_val<float> z;
};
CHECK_OFFSET(VPADVec3D, 0x00, x);
CHECK_OFFSET(VPADVec3D, 0x04, y);
CHECK_OFFSET(VPADVec3D, 0x08, z);
CHECK_SIZE(VPADVec3D, 0x0C);

struct VPADAccStatus
{
   be2_struct<VPADVec3D> acc;
   be2_val<float> magnitude;
   be2_val<float> variation;
   be2_struct<VPADVec2D> vertical;
};
CHECK_OFFSET(VPADAccStatus, 0x00, acc);
CHECK_OFFSET(VPADAccStatus, 0x0C, magnitude);
CHECK_OFFSET(VPADAccStatus, 0x10, variation);
CHECK_OFFSET(VPADAccStatus, 0x14, vertical);
CHECK_SIZE(VPADAccStatus, 0x1c);

struct VPADDirection
{
   be2_struct<VPADVec3D> x;
   be2_struct<VPADVec3D> y;
   be2_struct<VPADVec3D> z;
};
CHECK_OFFSET(VPADDirection, 0x00, x);
CHECK_OFFSET(VPADDirection, 0x0C, y);
CHECK_OFFSET(VPADDirection, 0x18, z);
CHECK_SIZE(VPADDirection, 0x24);

struct VPADGyroStatus
{
   be2_val<float> unk1;
   be2_val<float> unk2;
   be2_val<float> unk3;
   be2_val<float> unk4;
   be2_val<float> unk5;
   be2_val<float> unk6;
};
CHECK_OFFSET(VPADGyroStatus, 0x00, unk1);
CHECK_OFFSET(VPADGyroStatus, 0x04, unk2);
CHECK_OFFSET(VPADGyroStatus, 0x08, unk3);
CHECK_OFFSET(VPADGyroStatus, 0x0C, unk4);
CHECK_OFFSET(VPADGyroStatus, 0x10, unk5);
CHECK_OFFSET(VPADGyroStatus, 0x14, unk6);
CHECK_SIZE(VPADGyroStatus, 0x18);

struct VPADTouchData
{
   //! The x-coordinate of a touched point.
   be2_val<uint16_t> x;

   //! The y-coordinate of a touched point.
   be2_val<uint16_t> y;

   //! 0 if screen is not currently being touched
   be2_val<uint16_t> touched;

   //! Bitfield of #VPADTouchPadValidity to indicate how touch sample accuracy
   be2_val<uint16_t> validity;
};
CHECK_OFFSET(VPADTouchData, 0x00, x);
CHECK_OFFSET(VPADTouchData, 0x02, y);
CHECK_OFFSET(VPADTouchData, 0x04, touched);
CHECK_OFFSET(VPADTouchData, 0x06, validity);
CHECK_SIZE(VPADTouchData, 0x08);

struct VPADStatus
{
   //! Indicates what buttons are held down
   be2_val<VPADButtons> hold;

   //! Indicates what buttons have been pressed since last sample
   be2_val<VPADButtons> trigger;

   //! Indicates what buttons have been released since last sample
   be2_val<VPADButtons> release;

   //! Position of left analog stick
   be2_struct<VPADVec2D> leftStick;

   //! Position of right analog stick
   be2_struct<VPADVec2D> rightStick;

   //! Status of DRC accelorometer
   be2_struct<VPADAccStatus> accelorometer;

   //! Status of DRC gyro
   be2_struct<VPADGyroStatus> gyro;

   UNKNOWN(0x02);

   //! Current touch position on DRC
   be2_struct<VPADTouchData> tpNormal;

   //! Filtered touch position, first level of smoothing
   be2_struct<VPADTouchData> tpFiltered1;

   //! Filtered touch position, second level of smoothing
   be2_struct<VPADTouchData> tpFiltered2;

   UNKNOWN(0x28);

   //! Status of DRC magnetometer
   be2_struct<VPADVec3D> mag;

   //! Current volume set by the slide control
   be2_val<uint8_t> slideVolume;

   //! Battery level of controller
   be2_val<uint8_t> battery;

   //! Status of DRC microphone
   be2_val<uint8_t> micStatus;

   //! Unknown volume related value
   be2_val<uint8_t> slideVolumeEx;

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

void
VPADInit();

void
VPADSetAccParam(VPADChan chan,
                float unk1,
                float unk2);

void
VPADSetBtnRepeat(VPADChan chan,
                 float unk1,
                 float unk2);

uint32_t
VPADRead(VPADChan chan,
         virt_ptr<VPADStatus> buffers,
         uint32_t bufferCount,
         virt_ptr<VPADReadError> outError);

void
VPADGetTPCalibratedPoint(VPADChan chan,
                         virt_ptr<VPADTouchData> calibratedData,
                         virt_ptr<VPADTouchData> uncalibratedData);

bool
VPADBASEGetHeadphoneStatus(VPADChan chan);

/** @} */

} // namespace cafe::vpad
