#pragma once
#include "padscore_enum.h"
#include <libcpu/be2_struct.h>

namespace cafe::padscore
{

#pragma pack(push, 1)

using  WPADConnectCallback = virt_func_ptr<
   void (WPADChan chan, WPADError error)>;

using  WPADExtensionCallback = virt_func_ptr<
   void (WPADChan chan, WPADError error)>;

using  WPADSamplingCallback = virt_func_ptr<
   void (WPADChan chan)>;

struct WPADVec2D
{
   be2_val<int16_t> x;
   be2_val<int16_t> y;
};
CHECK_OFFSET(WPADVec2D, 0x00, x);
CHECK_OFFSET(WPADVec2D, 0x02, y);
CHECK_SIZE(WPADVec2D, 0x04);

struct WPADStatus
{
   UNKNOWN(0x28);
   be2_val<WPADExtensionType> extensionType;
   be2_val<int8_t> err;
   PADDING(2);
};
CHECK_OFFSET(WPADStatus, 0x28, extensionType);
CHECK_OFFSET(WPADStatus, 0x29, err);
CHECK_SIZE(WPADStatus, 0x2C);

struct WPADStatusProController
{
   be2_struct<WPADStatus> base;
   be2_val<uint32_t> buttons;
   be2_struct<WPADVec2D> leftStick;
   be2_struct<WPADVec2D> rightStick;
   UNKNOWN(8);
   be2_val<WPADDataFormat> dataFormat;
   PADDING(3);
};
CHECK_OFFSET(WPADStatusProController, 0x00, base);
CHECK_OFFSET(WPADStatusProController, 0x2C, buttons);
CHECK_OFFSET(WPADStatusProController, 0x30, leftStick);
CHECK_OFFSET(WPADStatusProController, 0x34, rightStick);
CHECK_OFFSET(WPADStatusProController, 0x40, dataFormat);
CHECK_SIZE(WPADStatusProController, 0x44);

void
WPADInit();

WPADLibraryStatus
WPADGetStatus();

void
WPADShutdown();

void
WPADControlMotor(WPADChan chan,
                 WPADMotorCommand command);

void
WPADDisconnect(WPADChan chan);

void
WPADEnableURCC(BOOL enable);

void
WPADEnableWiiRemote(BOOL enable);

WPADBatteryLevel
WPADGetBatteryLevel(WPADChan chan);

int8_t
WPADGetSpeakerVolume();

WPADError
WPADProbe(WPADChan chan,
          virt_ptr<WPADExtensionType> outExtensionType);

void
WPADRead(WPADChan chan,
         virt_ptr<void> data);

void
WPADSetAutoSleepTime(uint8_t time);

WPADConnectCallback
WPADSetConnectCallback(WPADChan chan,
                       WPADConnectCallback callback);

WPADError
WPADSetDataFormat(WPADChan chan,
                  WPADDataFormat format);

WPADExtensionCallback
WPADSetExtensionCallback(WPADChan chan,
                         WPADExtensionCallback callback);

WPADSamplingCallback
WPADSetSamplingCallback(WPADChan chan,
                        WPADSamplingCallback callback);

#pragma pack(pop)

} // namespace cafe::padscore
