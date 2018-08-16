#include "padscore.h"
#include "padscore_wpad.h"

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
   decaf_warn_stub();
}

void
WPADEnableWiiRemote(BOOL enable)
{
   decaf_warn_stub();
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
   decaf_warn_stub();
   if (chan < WPADChan::NumChans) {
      sWpadData->chanData[chan].dataFormat = format;
   }

   return WPADError::NoController;
}

WPADExtensionCallback
WPADSetExtensionCallback(WPADChan chan,
                         WPADExtensionCallback callback)
{
   decaf_warn_stub();
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
   decaf_warn_stub();
   if (chan >= WPADChan::NumChans) {
      return nullptr;
   }

   auto prev = sWpadData->chanData[chan].samplingCallback;
   sWpadData->chanData[chan].samplingCallback = callback;
   return prev;
}

void
Library::registerWpadSymbols()
{
   RegisterFunctionExport(WPADInit);
   RegisterFunctionExport(WPADGetStatus);
   RegisterFunctionExport(WPADShutdown);
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
   RegisterFunctionExport(WPADSetExtensionCallback);
   RegisterFunctionExport(WPADSetSamplingCallback);

   RegisterDataInternal(sWpadData);
}


} // namespace cafe::padscore
