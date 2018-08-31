#pragma once
#include "padscore_enum.h"
#include "padscore_wpad.h"
#include <libcpu/be2_struct.h>

namespace cafe::padscore
{

#pragma pack(push, 1)

using KPADChan = WPADChan;
using KPADDataFormat = WPADDataFormat;
using KPADExtensionType = WPADExtensionType;

struct KPADVec2D
{
   be2_val<float> x;
   be2_val<float> y;
};
CHECK_OFFSET(KPADVec2D, 0x00, x);
CHECK_OFFSET(KPADVec2D, 0x04, y);
CHECK_SIZE(KPADVec2D, 0x08);

struct KPADExtClassicStatus
{
   be2_val<uint32_t> hold;
   be2_val<uint32_t> trigger;
   be2_val<uint32_t> release;
   be2_struct<KPADVec2D> leftStick;
   be2_struct<KPADVec2D> rightStick;
   be2_val<float> leftTrigger;
   be2_val<float> rightTrigger;
};
CHECK_OFFSET(KPADExtClassicStatus, 0x00, hold);
CHECK_OFFSET(KPADExtClassicStatus, 0x04, trigger);
CHECK_OFFSET(KPADExtClassicStatus, 0x08, release);
CHECK_OFFSET(KPADExtClassicStatus, 0x0C, leftStick);
CHECK_OFFSET(KPADExtClassicStatus, 0x14, rightStick);
CHECK_OFFSET(KPADExtClassicStatus, 0x1C, leftTrigger);
CHECK_OFFSET(KPADExtClassicStatus, 0x20, rightTrigger);

struct KPADExtNunchukStatus
{
   be2_struct<KPADVec2D> stick;
};
CHECK_OFFSET(KPADExtNunchukStatus, 0x00, stick);

struct KPADExtProControllerStatus
{
   be2_val<uint32_t> hold;
   be2_val<uint32_t> trigger;
   be2_val<uint32_t> release;
   be2_struct<KPADVec2D> leftStick;
   be2_struct<KPADVec2D> rightStick;
   be2_val<int32_t> charging;
   be2_val<int32_t> wired;
};
CHECK_OFFSET(KPADExtProControllerStatus, 0x00, hold);
CHECK_OFFSET(KPADExtProControllerStatus, 0x04, trigger);
CHECK_OFFSET(KPADExtProControllerStatus, 0x08, release);
CHECK_OFFSET(KPADExtProControllerStatus, 0x0C, leftStick);
CHECK_OFFSET(KPADExtProControllerStatus, 0x14, rightStick);
CHECK_OFFSET(KPADExtProControllerStatus, 0x1C, charging);
CHECK_OFFSET(KPADExtProControllerStatus, 0x20, wired);

struct KPADExtStatus
{
   union
   {
      be2_struct<KPADExtClassicStatus> classic;
      be2_struct<KPADExtNunchukStatus> nunchuk;
      be2_struct<KPADExtProControllerStatus> proController;
      PADDING(0x50);
   };
};
CHECK_SIZE(KPADExtStatus, 0x50);

struct KPADStatus
{
   //! Indicates what KPADButtons are held down
   be2_val<uint32_t> hold;

   //! Indicates what KPADButtons have been pressed since last sample
   be2_val<uint32_t> trigger;

   //! Indicates what KPADButtons have been released since last sample
   be2_val<uint32_t> release;

   UNKNOWN(5 * 4);
   be2_struct<KPADVec2D> pos;
   UNKNOWN(3 * 4);
   be2_struct<KPADVec2D> angle;
   UNKNOWN(8 * 4);

   //! Type of data stored in extStatus
   be2_val<KPADExtensionType> extensionType;

   //! Value from KPADError
   be2_val<int8_t> error;

   be2_val<uint8_t> posValid;
   be2_val<KPADDataFormat> format;

   // Extension data, check with extensionType to see what is valid to read
   be2_struct<KPADExtStatus> extStatus;

   UNKNOWN(16 * 4);
};
CHECK_OFFSET(KPADStatus, 0x00, hold);
CHECK_OFFSET(KPADStatus, 0x04, trigger);
CHECK_OFFSET(KPADStatus, 0x08, release);
CHECK_OFFSET(KPADStatus, 0x20, pos);
CHECK_OFFSET(KPADStatus, 0x34, angle);
CHECK_OFFSET(KPADStatus, 0x5C, extensionType);
CHECK_OFFSET(KPADStatus, 0x5D, error);
CHECK_OFFSET(KPADStatus, 0x5E, posValid);
CHECK_OFFSET(KPADStatus, 0x5F, format);
CHECK_OFFSET(KPADStatus, 0x60, extStatus);
CHECK_SIZE(KPADStatus, 0xF0);

#pragma pack(pop)

void
KPADInit();

void
KPADInitEx(virt_ptr<void> a1,
           uint32_t a2);

void
KPADShutdown();

void
KPADEnableDPD(KPADChan chan);

void
KPADDisableDPD(KPADChan chan);

uint32_t
KPADGetMplsWorkSize();

void
KPADSetMplsWorkarea(virt_ptr<void> buffer);

int32_t
KPADRead(KPADChan chan,
         virt_ptr<KPADStatus> data,
         uint32_t size);

int32_t
KPADReadEx(KPADChan chan,
           virt_ptr<KPADStatus> data,
           uint32_t size,
           virt_ptr<KPADReadError> outError);

} // namespace cafe::padscore
