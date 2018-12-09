#include "padscore.h"
#include "padscore_wpad.h"
#include "padscore_kpad.h"

#include "input/input.h"

#include "cafe/libraries/cafe_hle_stub.h"

namespace cafe::padscore
{

struct WpadData
{
   struct ChanData
   {
      be2_val<WPADDataFormat> dataFormat;
      be2_val<WPADConnectCallback> connectCallback;
      be2_val<WPADExtensionCallback> extensionCallback;
      be2_val<WPADSamplingCallback> samplingCallback;
   };

   be2_val<WPADLibraryStatus> status;
   be2_val<BOOL> proControllerAllowed;
   be2_val<BOOL> wiiMoteAllowed;
   be2_array<ChanData, WPADChan::NumChans> chanData;
};

static virt_ptr<WpadData>
sWpadData = nullptr;

void
WPADInit()
{
   sWpadData->status = WPADLibraryStatus::Initialised;
}

WPADLibraryStatus
WPADGetStatus()
{
   return sWpadData->status;
}

void
WPADShutdown()
{
   sWpadData->status = WPADLibraryStatus::Uninitialised;
}

BOOL 
WPADIsDpdEnabled(WPADChan chan)
{
   decaf_warn_stub();
   return FALSE;
}

void
WPADControlMotor(WPADChan chan,
                 WPADMotorCommand command)
{
   decaf_warn_stub();
}

void
WPADDisconnect(WPADChan chan)
{
   decaf_warn_stub();
}

void
WPADEnableURCC(BOOL enable)
{
   sWpadData->proControllerAllowed = enable;
}

void
WPADEnableWiiRemote(BOOL enable)
{
   sWpadData->wiiMoteAllowed = enable;
}

WPADBatteryLevel
WPADGetBatteryLevel(WPADChan chan)
{
   decaf_warn_stub();
   return WPADBatteryLevel::High;
}

int8_t
WPADGetSpeakerVolume()
{
   decaf_warn_stub();
   return 0;
}

WPADError
WPADProbe(WPADChan chan,
          virt_ptr<WPADExtensionType> outExtensionType)
{
   decaf_warn_stub();

   if (outExtensionType) {
      *outExtensionType = WPADExtensionType::NoController;
   }

   return WPADError::NoController;
}

void
WPADRead(WPADChan chan,
         virt_ptr<void> data)
{
   decaf_warn_stub();

   if (data) {
      auto baseStatus = virt_cast<WPADStatus *>(data);
      baseStatus->err = static_cast<int8_t>(WPADError::NoController);
      baseStatus->extensionType = WPADExtensionType::NoController;
   }
}

void
WPADSetAutoSleepTime(uint8_t time)
{
   decaf_warn_stub();
}

WPADConnectCallback
WPADSetConnectCallback(WPADChan chan,
                       WPADConnectCallback callback)
{
   decaf_warn_stub();
   if (chan >= WPADChan::NumChans) {
      return nullptr;
   }

   auto prev = sWpadData->chanData[chan].connectCallback;
   sWpadData->chanData[chan].connectCallback = callback;
   return prev;
}

WPADError
WPADSetDataFormat(WPADChan chan,
                  WPADDataFormat format)
{
   if (chan < WPADChan::NumChans) {
      sWpadData->chanData[chan].dataFormat = format;
   }

   auto channel = static_cast<input::wpad::Channel>(chan);

   if (input::getControllerType(channel) == input::wpad::Type::Disconnected) {
      return WPADError::NoController;
   }
   return WPADError::OK;
}

WPADDataFormat
WPADGetDataFormat(WPADChan chan)
{
   if (chan < WPADChan::NumChans) {
      return sWpadData->chanData[chan].dataFormat;
   }

  decaf_abort("Unsupported controller channel")
}

WPADExtensionCallback
WPADSetExtensionCallback(WPADChan chan,
                         WPADExtensionCallback callback)
{
   if (chan >= WPADChan::NumChans) {
      return nullptr;
   }

   auto prev = sWpadData->chanData[chan].extensionCallback;
   sWpadData->chanData[chan].extensionCallback = callback;
   return prev;
}

WPADSamplingCallback
WPADSetSamplingCallback(WPADChan chan,
                        WPADSamplingCallback callback)
{
   if (chan >= WPADChan::NumChans) {
      return nullptr;
   }

   auto prev = sWpadData->chanData[chan].samplingCallback;
   sWpadData->chanData[chan].samplingCallback = callback;
   return prev;
}

WPADError 
WPADControlDpd(int32_t chan,
               uint32_t command,
               WPADCallback callback)
{
   decaf_warn_stub();
   return WPADError::OK;
}

bool
ProControllerIsAllowed()
{
   return sWpadData->proControllerAllowed;
}
void
Library::registerWpadSymbols()
{
   RegisterFunctionExport(WPADInit);
   RegisterFunctionExport(WPADGetStatus);
   RegisterFunctionExport(WPADShutdown);
   RegisterFunctionExport(WPADIsDpdEnabled);
   RegisterFunctionExport(WPADControlMotor);
   RegisterFunctionExport(WPADDisconnect);
   RegisterFunctionExport(WPADEnableURCC);
   RegisterFunctionExport(WPADEnableWiiRemote);
   RegisterFunctionExport(WPADGetBatteryLevel);
   RegisterFunctionExport(WPADGetSpeakerVolume);
   RegisterFunctionExport(WPADProbe);
   RegisterFunctionExport(WPADRead);
   RegisterFunctionExport(WPADSetAutoSleepTime);
   RegisterFunctionExport(WPADSetConnectCallback);
   RegisterFunctionExport(WPADSetDataFormat);
   RegisterFunctionExport(WPADGetDataFormat);
   RegisterFunctionExport(WPADSetExtensionCallback);
   RegisterFunctionExport(WPADSetSamplingCallback);

   RegisterDataInternal(sWpadData);
}


} // namespace cafe::padscore
