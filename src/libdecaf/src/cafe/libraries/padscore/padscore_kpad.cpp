#include "padscore.h"
#include "padscore_kpad.h"
#include "padscore_wpad.h"

#include "input/input.h"

#include "cafe/cafe_stackobject.h"
#include "cafe/libraries/cafe_hle_stub.h"
#include "cafe/libraries/coreinit/coreinit_ios.h"

namespace cafe::padscore
{

static const std::vector<std::pair<WPADProButton, input::wpad::Pro>>
   gButtonMap =
{
   { WPADProButton::Up,       input::wpad::Pro::Up },
   { WPADProButton::Left,     input::wpad::Pro::Left },
   { WPADProButton::ZR,       input::wpad::Pro::TriggerZR },
   { WPADProButton::X,        input::wpad::Pro::X },
   { WPADProButton::A,        input::wpad::Pro::A },
   { WPADProButton::Y,        input::wpad::Pro::Y },
   { WPADProButton::B,        input::wpad::Pro::B },
   { WPADProButton::ZL,       input::wpad::Pro::TriggerZL },
   { WPADProButton::R,        input::wpad::Pro::TriggerR },
   { WPADProButton::Plus,     input::wpad::Pro::Plus },
   { WPADProButton::Home,     input::wpad::Pro::Home },
   { WPADProButton::Minus,    input::wpad::Pro::Minus },
   { WPADProButton::L,        input::wpad::Pro::TriggerL },
   { WPADProButton::Down,     input::wpad::Pro::Down },
   { WPADProButton::Right,    input::wpad::Pro::Right },
   { WPADProButton::StickL,   input::wpad::Pro::LeftStick },
   { WPADProButton::StickR,   input::wpad::Pro::RightStick },
};

struct KpadData
{
   struct ChanData
   {
      be2_val<WPADDataFormat> dataFormat;
      be2_val<WPADConnectCallback> connectCallback;
      be2_val<WPADExtensionCallback> extensionCallback;
      be2_val<WPADSamplingCallback> samplingCallback;
      be2_val<KPADControlDpdCallback> controlDpdCallback;
      be2_val<KPADControlMplsCallback > controlMplsCallback;
      be2_val<uint8_t> procMode;
      be2_val<KPADPlayMode> playMode;
      be2_val<KPADPlayMode> posPlayMode;
      be2_val<KPADPlayMode> horiPlayMode;
      be2_val<KPADPlayMode> distPlayMode;
      be2_val<KPADMplsZeroDriftMode> zeroPointDriftMode;
      be2_val<BOOL> aimingMode;
      be2_val<BOOL> mplsMode;
      be2_val<BOOL> mplsAccRevise;
      be2_val<BOOL> mplsDirRevise;
      be2_val<BOOL> mplsDpdRevise;
      be2_val<BOOL> mplsZeroPlay;
      be2_val<float> delay_sec;
      be2_val<float> pulse_sec;
      be2_val<float> play_radius;
      be2_val<float> sensitivity;
      be2_val<float> posPlay_radius;
      be2_val<float> posSensitivity;
      be2_val<float> horiPlay_radius;
      be2_val<float> horiSensitivity;
      be2_val<float> distPlay_radius;
      be2_val<float> distSensitivity;
      be2_val<float> ax;
      be2_val<float> ay;
      be2_val<float> az;
      be2_val<float> sensorHeight;
      be2_val<float> revise_pw;
      be2_val<float> revise_range;
      be2_val<float> dir_revise_pw;
      be2_val<float> dpd_revise_pw;
      be2_val<float> zero_play_radius;
      be2_val<BOOL> calibrating;
   };

   be2_val<WPADLibraryStatus> status;
   be2_array<ChanData, WPADChan::NumChans> chanData;
};

static virt_ptr<KpadData>
sKpadData;

static uint32_t
gLastButtonState = 0;

void
KPADInit()
{
   decaf_warn_stub();
   KPADInitEx(NULL, 0);
}

void
KPADInitEx(virt_ptr<void> a1,
           uint32_t a2)
{
   decaf_warn_stub();
   WPADInit();
}

void
KPADShutdown()
{
   decaf_warn_stub();
}

void 
KPADReset()
{
   decaf_warn_stub();
}


WPADConnectCallback 
KPADSetConnectCallback(KPADChan chan, 
                       WPADConnectCallback callback)
{
   decaf_warn_stub();
   if (chan >= KPADChan::NumChans) {
      return nullptr;
   }

   auto prev = sKpadData->chanData[chan].connectCallback;
   sKpadData->chanData[chan].connectCallback = callback;
   return prev;
}


//! Enable "Direct Pointing Device"
void 
KPADEnableDPD(KPADChan chan)
{
   decaf_warn_stub();
   WPADControlDpd(chan, 0, 0);
   WPADSetDataFormat(chan, WPADDataFormat::ProController);
}

//! Disable "Direct Pointing Device"
void
KPADDisableDPD(KPADChan chan)
{
   decaf_warn_stub();
   WPADControlDpd(chan, 0, 0);
   WPADSetDataFormat(chan, WPADDataFormat::ProController);
}

uint32_t
KPADGetMplsWorkSize()
{
   return 0x5FE0;
}

void
KPADSetMplsWorkarea(virt_ptr<void> buffer)
{
   decaf_warn_stub();
}

int32_t
KPADRead(KPADChan chan,
         virt_ptr<KPADStatus> data,
         uint32_t size)
{
   decaf_warn_stub();
   StackObject<KPADReadError> readError;
   auto result = KPADReadEx(chan, data, size, readError);

   if (*readError != KPADReadError::Success) {
      return *readError;
   } else {
      return result;
   }
}

int32_t
KPADReadEx(KPADChan chan,
           virt_ptr<KPADStatus> buffers,
           uint32_t bufferCount,
           virt_ptr<KPADReadError> outError)
{
   if (bufferCount < 1) {
      if (outError) {
         *outError = KPADReadError::NoSamples;
      }

      return 0;
   }

   if (chan >= input::wpad::MaxControllers) {
      if (outError) {
         *outError = KPADReadError::InvalidController;
      }

      return 0;
   }

   memset(virt_addrof(buffers[0]).getRawPointer(), 0, sizeof(KPADStatus));

   auto channel = static_cast<input::wpad::Channel>(chan);
   auto &buffer = buffers[0];

   // Update button state
   for (auto &pair : gButtonMap) {
      auto bit = pair.first;
      auto button = pair.second;
      auto status = input::getButtonStatus(channel, button);
      auto previous = gLastButtonState & bit;

      if (status == input::ButtonStatus::ButtonPressed) {
         if (!previous) {
            buffer.trigger |= bit;
         }

         buffer.hold |= bit;
      }
      else if (previous) {
         buffer.release |= bit;
      }
   }

   gLastButtonState = buffer.hold;

   // Update axis state
   buffer.pos.x = input::getAxisValue(channel, input::wpad::ProAxis::LeftStickX);
   buffer.pos.y = input::getAxisValue(channel, input::wpad::ProAxis::LeftStickY);
   buffer.angle.x = input::getAxisValue(channel, input::wpad::ProAxis::RightStickX);
   buffer.angle.y = input::getAxisValue(channel, input::wpad::ProAxis::RightStickY);

   if (outError) {
      *outError = KPADReadError::Success;
   }

   return 1;
}

WPADSamplingCallback
KPADSetSamplingCallback(KPADChan chan,
                        WPADSamplingCallback callback)
{
   decaf_warn_stub();
   if (chan >= KPADChan::NumChans) {
      return nullptr;
   }

   auto prev = sKpadData->chanData[chan].samplingCallback;
   sKpadData->chanData[chan].samplingCallback = callback;
   return prev;
}

void 
KPADSetBtnRepeat(KPADChan chan, 
                 float delay_sec, 
                 float pulse_sec)
{
   decaf_warn_stub();
   if (chan >= KPADChan::NumChans) {
      return;
   }
   sKpadData->chanData[chan].delay_sec = delay_sec;
   sKpadData->chanData[chan].pulse_sec = pulse_sec;
}

void
KPADSetButtonProcMode(KPADChan chan, 
                      uint8_t mode)
{
   decaf_warn_stub();
   if (chan >= KPADChan::NumChans) {
      return;
   }
   sKpadData->chanData[chan].procMode = mode; 
}


uint8_t
KPADGetButtonProcMode(KPADChan chan)
{
   if (chan >= KPADChan::NumChans) {
      return 0;
   } 
   return sKpadData->chanData[chan].procMode;
}

void 
KPADSetAccParam(KPADChan chan, 
                float play_radius, 
                float sensitivity)
{
   decaf_warn_stub();
   if (chan >= KPADChan::NumChans) {
      return;
   }
   sKpadData->chanData[chan].play_radius = play_radius;
   sKpadData->chanData[chan].sensitivity = sensitivity;
}

void 
KPADGetAccParam(KPADChan chan, 
                virt_ptr<float> play_radius, 
                virt_ptr<float> sensitivity)
{
   decaf_warn_stub();
   if (chan >= KPADChan::NumChans) {
      return;
   }
   *play_radius = sKpadData->chanData[chan].play_radius;
   *sensitivity = sKpadData->chanData[chan].sensitivity;
}


void 
KPADSetAccPlayMode(KPADChan chan, 
                   KPADPlayMode mode)
{
   decaf_warn_stub();
   if (chan >= KPADChan::NumChans) {
      return;
   }
   sKpadData->chanData[chan].playMode = mode;
}


KPADPlayMode 
KPADGetAccPlayMode(KPADChan chan)
{
   if (chan >= KPADChan::NumChans) {
      return KPADPlayMode::KPAD_PLAY_MODE_LOOSE;
   }
   return sKpadData->chanData[chan].playMode;
}

float 
KPADReviseAcc(virt_ptr<ios::IoctlVec> acc)
{
   return 0.0;
}

void 
KPADSetControlDpdCallback(KPADChan chan, 
                          KPADControlDpdCallback callback)
{
   if (chan >= KPADChan::NumChans) {
      return;
   }
   sKpadData->chanData[chan].controlDpdCallback = callback;
}

int32_t
KPADCalibrateDPD(KPADChan  chan)
{
   return 2;
}

void
KPADGetProjectionPos(virt_ptr<KPADVec2D> dst,
                     const virt_ptr<KPADVec2D> src,
                     const virt_ptr<KPADRect> projRect,
                     float viRatio)
{
   decaf_warn_stub();
}

void 
KPADEnableAimingMode(KPADChan chan)
{
   if (chan >= KPADChan::NumChans) {
      return;
   }
   sKpadData->chanData[chan].aimingMode = TRUE;
}

void 
KPADDisableAimingMode(KPADChan chan)
{
   if (chan >= KPADChan::NumChans) {
      return;
   }
   sKpadData->chanData[chan].aimingMode = FALSE;
}

BOOL 
KPADIsEnableAimingMode(KPADChan chan)
{
   if (chan >= KPADChan::NumChans) {
      return FALSE;
   }
   return sKpadData->chanData[chan].aimingMode;
}

void 
KPADSetPosPlayMode(KPADChan chan,
                   KPADPlayMode mode)
{
   if (chan >= KPADChan::NumChans) {
      return;
   }
   sKpadData->chanData[chan].posPlayMode = mode;
}

KPADPlayMode
KPADGetPosPlayMode(KPADChan chan)
{
   if (chan >= KPADChan::NumChans) {
      return KPADPlayMode::KPAD_PLAY_MODE_LOOSE;
   }
   return sKpadData->chanData[chan].posPlayMode ;
}

void
KPADSetPosParam(KPADChan chan,
                float play_radius,
                float sensitivity)
{
   decaf_warn_stub();
   if (chan >= KPADChan::NumChans) {
      return;
   }
   sKpadData->chanData[chan].posPlay_radius = play_radius;
   sKpadData->chanData[chan].posSensitivity = sensitivity;
}

void
KPADGetPosParam(KPADChan chan,
                virt_ptr<float> play_radius,
                virt_ptr<float> sensitivity)
{
   decaf_warn_stub();
   if (chan >= KPADChan::NumChans) {
      return;
   }
   *play_radius = sKpadData->chanData[chan].posPlay_radius;
   *sensitivity = sKpadData->chanData[chan].posSensitivity;
}

void
KPADSetHoriPlayMode(KPADChan chan, 
                    KPADPlayMode mode)
{
   if (chan >= KPADChan::NumChans) {
      return;
   }
   sKpadData->chanData[chan].horiPlayMode = mode;
}

KPADPlayMode
KPADGetHoriPlayMode(KPADChan chan)
{
   if (chan >= KPADChan::NumChans) {
      return KPADPlayMode::KPAD_PLAY_MODE_LOOSE;
   }
   return sKpadData->chanData[chan].horiPlayMode;
}

void
KPADSetHoriParam(KPADChan chan,
                 float play_radius,
                 float sensitivity)
{
   decaf_warn_stub();
   if (chan >= KPADChan::NumChans) {
      return;
   }
   sKpadData->chanData[chan].horiPlay_radius = play_radius;
   sKpadData->chanData[chan].horiSensitivity = sensitivity;
}

void
KPADGetHoriParam(KPADChan chan,
                 virt_ptr<float> play_radius,
                 virt_ptr<float> sensitivity)
{
   decaf_warn_stub();
   if (chan >= KPADChan::NumChans) {
      return;
   }
   *play_radius = sKpadData->chanData[chan].horiPlay_radius;
   *sensitivity = sKpadData->chanData[chan].horiSensitivity;
}

void
KPADSetDistPlayMode(KPADChan chan, 
                    KPADPlayMode mode)
{
   if (chan >= KPADChan::NumChans) {
      return;
   }
   sKpadData->chanData[chan].distPlayMode = mode;
}

KPADPlayMode
KPADGetDistPlayMode(KPADChan chan)
{
   if (chan >= KPADChan::NumChans) {
      return KPADPlayMode::KPAD_PLAY_MODE_LOOSE;
   }
   return sKpadData->chanData[chan].distPlayMode;
}

void
KPADSetDistParam(KPADChan chan, 
                 float play_radius, 
                 float sensitivity)
{
   decaf_warn_stub();
   if (chan >= KPADChan::NumChans) {
      return;
   }
   sKpadData->chanData[chan].distPlay_radius = play_radius;
   sKpadData->chanData[chan].distSensitivity = sensitivity;
}

void
KPADGetDistParam(KPADChan chan,
                 virt_ptr<float> play_radius, 
                 virt_ptr<float> sensitivity)
{
   decaf_warn_stub();
   if (chan >= KPADChan::NumChans) {
      return;
   }
   *play_radius = sKpadData->chanData[chan].distPlay_radius;
   *sensitivity = sKpadData->chanData[chan].distSensitivity;
}

void 
KPADSetSensorHeight(KPADChan chan, 
                    float level)
{
   decaf_warn_stub();
   if (chan >= KPADChan::NumChans) {
      return;
   }
   sKpadData->chanData[chan].sensorHeight = level;
}

float 
KPADGetSensorHeight(KPADChan chan)
{
   if (chan >= KPADChan::NumChans) {
      return 0.0;
   }
   return sKpadData->chanData[chan].sensorHeight;
}

void 
KPADEnableMpls(KPADChan chan, 
               KPADMplsMode mode)
{
   if (chan >= KPADChan::NumChans) {
      return;
   }
   sKpadData->chanData[chan].mplsMode = mode;
}

void KPADDisableMpls(KPADChan chan)
{
   if (chan >= KPADChan::NumChans) {
      return;
   }
}

void KPADSetControlMplsCallback(KPADChan chan, 
                                KPADControlMplsCallback callback)
{
   if (chan >= KPADChan::NumChans) {
      return;
   }
   sKpadData->chanData[chan].controlDpdCallback = callback;
}

uint8_t 
KPADGetMplsStatus(KPADChan chan)
{
   return 0;
}

void 
KPADStartMplsCalibration(KPADChan chan)
{
   if (chan >= KPADChan::NumChans) {
      return;
   }
   sKpadData->chanData[chan].calibrating = TRUE;
}

float KPADWorkMplsCalibration(KPADChan chan)
{
   if (chan >= KPADChan::NumChans) {
      return 0;
   }
   if (sKpadData->chanData[chan].calibrating)
   {
      return 0.5;
   }
   return -1.0;
}

void
KPADStopMplsCalibration(KPADChan chan)
{
   if (chan >= KPADChan::NumChans) {
      return;
   }
   sKpadData->chanData[chan].calibrating = FALSE;
}

void 
KPADResetMpls(KPADChan chan)
{
   if (chan >= KPADChan::NumChans) {
      return;
   }
}

void 
KPADSetMplsAngle(KPADChan chan, 
                 float ax, 
                 float ay, 
                 float az)
{
   if (chan >= KPADChan::NumChans) {
      return;
   }
   sKpadData->chanData[chan].ax = ax;
   sKpadData->chanData[chan].ay = ay;
   sKpadData->chanData[chan].az = az;
}

void 
KPADSetMplsDirection(KPADChan chan, 
                     virt_ptr<void> dir)
{
   if (chan >= KPADChan::NumChans) {
      return;
   }
}

void 
KPADSetMplsDirectionMag(KPADChan chan, 
                        float mag)
{
   if (chan >= KPADChan::NumChans) {
      return;
   }
}

void 
KPADEnableMplsAccRevise(KPADChan chan)
{
   if (chan >= KPADChan::NumChans) {
      return;
   }
   sKpadData->chanData[chan].mplsAccRevise = TRUE;
}

void KPADDisableMplsAccRevise(KPADChan chan)
{
   if (chan >= KPADChan::NumChans) {
      return;
   }
   sKpadData->chanData[chan].mplsAccRevise = FALSE;
}

float 
KPADIsEnableMplsAccRevise(KPADChan chan)
{
   if (chan >= KPADChan::NumChans) {
      return -1.0;
   }
   if (sKpadData->chanData[chan].mplsAccRevise)
   {
      return 0.5;
   }
   return -1.0;
}

void 
KPADInitMplsAccReviseParam(KPADChan chan)
{
   if (chan >= KPADChan::NumChans) {
      return;
   }
   sKpadData->chanData[chan].revise_pw = 0.030f;
   sKpadData->chanData[chan].revise_range = 0.400f;
}

void 
KPADSetMplsAccReviseParam(KPADChan chan, 
                          float revise_pw, 
                          float revise_range)
{
   if(chan >= KPADChan::NumChans) {
      return;
   }
   sKpadData->chanData[chan].revise_pw = revise_pw;
   sKpadData->chanData[chan].revise_range = revise_range;
}

void 
KPADGetMplsAccReviseParam(KPADChan chan, 
                          virt_ptr<float> revise_pw, 
                          virt_ptr<float>  revise_range)
{
   if (chan >= KPADChan::NumChans) {
      *revise_pw = 0.030f;
      *revise_range = 0.400f;
      return;
   }
   *revise_pw = sKpadData->chanData[chan].revise_pw;
   *revise_range = sKpadData->chanData[chan].revise_range;
}

void
KPADEnableMplsDirRevise(KPADChan chan)
{
   if (chan >= KPADChan::NumChans) {
      return;
   }
   sKpadData->chanData[chan].mplsDirRevise = TRUE;
}

void
KPADDisableMplsDirRevise(KPADChan chan)
{
   if (chan >= KPADChan::NumChans) {
      return;
   }
   sKpadData->chanData[chan].mplsDirRevise = FALSE;
}

float
KPADIsEnableMplsDirRevise(KPADChan chan)
{
   if (chan >= KPADChan::NumChans) {
      return -1.0;
   }
   if (sKpadData->chanData[chan].mplsDirRevise)
   {
      return 0.5;
   }
   return -1.0;
}

void
KPADInitMplsDirReviseParam(KPADChan chan)
{
   if (chan >= KPADChan::NumChans) {
      return;
   }
   sKpadData->chanData[chan].dir_revise_pw = 0.500f;
}

void
KPADSetMplsDirReviseParam(KPADChan chan, 
                          float revise_pw)
{
   if (chan >= KPADChan::NumChans) {
      return;
   }
   sKpadData->chanData[chan].dir_revise_pw = revise_pw;
}

void
KPADGetMplsDirReviseParam(KPADChan chan, 
                          virt_ptr<float> revise_pw)
{
   if (chan >= KPADChan::NumChans) {
      *revise_pw = 0.5f;
      return;
   }
   *revise_pw = sKpadData->chanData[chan].dir_revise_pw;
}

void
KPADSetMplsDirReviseBase(KPADChan chan, 
                         virt_ptr<float> revise_pw)
{
   if (chan >= KPADChan::NumChans) {
      *revise_pw = 0.5f;
      return;
   }
   *revise_pw = sKpadData->chanData[chan].dir_revise_pw;
}

void
KPADEnableMplsDpdRevise(KPADChan chan)
{
   if (chan >= KPADChan::NumChans) {
      return;
   }
   sKpadData->chanData[chan].mplsDpdRevise = TRUE;
}

void 
KPADDisableMplsDpdRevise(KPADChan chan)
{
   if (chan >= KPADChan::NumChans) {
      return;
   }
   sKpadData->chanData[chan].mplsDpdRevise = FALSE;
}

float
KPADIsEnableMplsDpdRevise(KPADChan chan)
{
   if (chan >= KPADChan::NumChans) {
      return -1.0;
   }
   if (sKpadData->chanData[chan].mplsDpdRevise)
   {
      return 0.5;
   }
   return -1.0;
}

void
KPADInitMplsDpdReviseParam(KPADChan chan)
{
   if (chan >= KPADChan::NumChans) {
      return;
   }
   sKpadData->chanData[chan].dpd_revise_pw = 0.500f;
}

void
KPADSetMplsDpdReviseParam(KPADChan chan, 
                          float revise_pw)
{
   if (chan >= KPADChan::NumChans) {
      return;
   }
   sKpadData->chanData[chan].dpd_revise_pw = revise_pw;
}

void
KPADGetMplsDpdReviseParam(KPADChan chan, 
                          virt_ptr<float> revise_pw)
{
   if (chan >= KPADChan::NumChans) {
      *revise_pw = 0.5f;
      return;
   }
   *revise_pw = sKpadData->chanData[chan].dpd_revise_pw;
}

void
KPADEnableMplsZeroPlay(KPADChan chan)
{
   if (chan >= KPADChan::NumChans) {
      return;
   }
   sKpadData->chanData[chan].mplsZeroPlay = TRUE;
}

void
KPADDisableMplsZeroPlay(KPADChan chan)
{
   if (chan >= KPADChan::NumChans) {
      return;
   }
   sKpadData->chanData[chan].mplsZeroPlay = FALSE;
}

float
KPADIsEnableMplsZeroPlay(KPADChan chan)
{
   if (chan >= KPADChan::NumChans) {
      return -1.0;
   }
   if (sKpadData->chanData[chan].mplsZeroPlay)
   {
      return 0.5;
   }
   return -1.0;
}

void
KPADInitMplsZeroPlayParam(KPADChan chan)
{
   if (chan >= KPADChan::NumChans) {
      return;
   }
   sKpadData->chanData[chan].zero_play_radius = 0.005f;
}

void
KPADSetMplsZeroPlayParam(KPADChan chan,
                         float zero_play_radius)
{
   if (chan >= KPADChan::NumChans) {
      return;
   }
   sKpadData->chanData[chan].zero_play_radius = zero_play_radius;
}

void
KPADGetMplsZeroPlayParam(KPADChan chan,
                         virt_ptr<float> zero_play_radius)
{
   if (chan >= KPADChan::NumChans) {
      *zero_play_radius = 0.005f;
      return;
   }
   *zero_play_radius = sKpadData->chanData[chan].zero_play_radius;
}

float
KPADIsEnableMplsZeroDrift(KPADChan chan)
{
   return -1.0;
}

void 
KPADInitMplsZeroDriftMode(KPADChan chan)
{
   if (chan >= KPADChan::NumChans) {
      return;
   }
   sKpadData->chanData[chan].zeroPointDriftMode = KPADMplsZeroDriftMode::KPAD_MPLS_ZERODRIFT_LOOSE;
}

void 
KPADSetMplsZeroDriftMode(KPADChan chan,
                         KPADMplsZeroDriftMode mode)
{
   if (chan >= KPADChan::NumChans) {
      return;
   }
   sKpadData->chanData[chan].zeroPointDriftMode = mode;
}

void 
KPADGetMplsZeroDriftMode(KPADChan chan,
                         virt_ptr<KPADMplsZeroDriftMode> mode)
{
   if (chan >= KPADChan::NumChans) {
      return;
   }
   *mode = sKpadData->chanData[chan].zeroPointDriftMode;
}

void 
KPADSetMplsMagnification(KPADChan chan, 
                         float pitch, 
                         float yaw, 
                         float roll)
{
   decaf_warn_stub();
}

void 
KPADEnableStickCrossClamp(void)
{
   decaf_warn_stub();
}

void KPADDisableStickCrossClamp(void)
{
   decaf_warn_stub();
}

void 
KPADSetReviseMode(KPADChan chan, BOOL sw)
{
   decaf_warn_stub();
}

void KPADSetFSStickClamp(int8_t min, int8_t max)
{
   decaf_warn_stub();
}

float KPADGetReviseAngle(void)
{
   return 0.0;
}

void
Library::registerKpadSymbols()
{
   RegisterFunctionExport(KPADInit);
   RegisterFunctionExport(KPADInitEx);
   RegisterFunctionExport(KPADShutdown);
   RegisterFunctionExport(KPADReset);

   RegisterFunctionExport(KPADSetConnectCallback);

   RegisterFunctionExport(KPADRead);
   RegisterFunctionExport(KPADReadEx);
   RegisterFunctionExport(KPADSetSamplingCallback);

   RegisterFunctionExport(KPADSetBtnRepeat);
   RegisterFunctionExport(KPADSetButtonProcMode);
   RegisterFunctionExport(KPADGetButtonProcMode);

   RegisterFunctionExport(KPADSetAccParam);
   RegisterFunctionExport(KPADGetAccParam);
   RegisterFunctionExport(KPADSetAccPlayMode);
   RegisterFunctionExport(KPADGetAccPlayMode);
   RegisterFunctionExport(KPADReviseAcc);

   RegisterFunctionExport(KPADEnableDPD);
   RegisterFunctionExport(KPADDisableDPD);
   RegisterFunctionExport(KPADSetControlDpdCallback);
   RegisterFunctionExport(KPADCalibrateDPD);
   RegisterFunctionExport(KPADGetProjectionPos);
   RegisterFunctionExport(KPADEnableAimingMode);
   RegisterFunctionExport(KPADDisableAimingMode);
   RegisterFunctionExport(KPADIsEnableAimingMode);
   RegisterFunctionExport(KPADSetPosPlayMode);
   RegisterFunctionExport(KPADGetPosPlayMode);
   RegisterFunctionExport(KPADSetPosParam);
   RegisterFunctionExport(KPADGetPosParam);
   RegisterFunctionExport(KPADSetHoriPlayMode);
   RegisterFunctionExport(KPADGetHoriPlayMode);
   RegisterFunctionExport(KPADSetHoriParam);
   RegisterFunctionExport(KPADGetHoriParam);
   RegisterFunctionExport(KPADSetDistPlayMode);
   RegisterFunctionExport(KPADGetDistPlayMode);
   RegisterFunctionExport(KPADSetDistParam);
   RegisterFunctionExport(KPADGetDistParam);
   RegisterFunctionExport(KPADSetSensorHeight);
   RegisterFunctionExport(KPADGetSensorHeight);
   
   RegisterFunctionExport(KPADEnableMpls);
   RegisterFunctionExport(KPADDisableMpls);
   RegisterFunctionExport(KPADSetControlMplsCallback);
   RegisterFunctionExport(KPADGetMplsStatus);
   RegisterFunctionExport(KPADSetMplsWorkarea);
   RegisterFunctionExport(KPADGetMplsWorkSize);
   RegisterFunctionExport(KPADStartMplsCalibration);
   RegisterFunctionExport(KPADWorkMplsCalibration);
   RegisterFunctionExport(KPADStopMplsCalibration);
   RegisterFunctionExport(KPADResetMpls);
   RegisterFunctionExport(KPADSetMplsAngle);
   RegisterFunctionExport(KPADSetMplsDirection);
   RegisterFunctionExport(KPADSetMplsDirectionMag);
   RegisterFunctionExport(KPADEnableMplsAccRevise);
   RegisterFunctionExport(KPADDisableMplsAccRevise);
   RegisterFunctionExport(KPADIsEnableMplsAccRevise);
   RegisterFunctionExport(KPADInitMplsAccReviseParam);
   RegisterFunctionExport(KPADSetMplsAccReviseParam);
   RegisterFunctionExport(KPADGetMplsAccReviseParam);
   RegisterFunctionExport(KPADEnableMplsDirRevise);
   RegisterFunctionExport(KPADDisableMplsDirRevise);
   RegisterFunctionExport(KPADIsEnableMplsDirRevise);
   RegisterFunctionExport(KPADInitMplsDirReviseParam);
   RegisterFunctionExport(KPADGetMplsDirReviseParam);
   RegisterFunctionExport(KPADSetMplsDirReviseParam);
   RegisterFunctionExport(KPADSetMplsDirReviseBase);
   RegisterFunctionExport(KPADEnableMplsDpdRevise);
   RegisterFunctionExport(KPADDisableMplsDpdRevise);
   RegisterFunctionExport(KPADIsEnableMplsDpdRevise);
   RegisterFunctionExport(KPADInitMplsDpdReviseParam);
   RegisterFunctionExport(KPADSetMplsDpdReviseParam);
   RegisterFunctionExport(KPADGetMplsDpdReviseParam);                  
   RegisterFunctionExport(KPADEnableMplsZeroPlay);
   RegisterFunctionExport(KPADDisableMplsZeroPlay);
   RegisterFunctionExport(KPADIsEnableMplsZeroPlay);
   RegisterFunctionExport(KPADInitMplsZeroPlayParam);
   RegisterFunctionExport(KPADSetMplsZeroPlayParam);
   RegisterFunctionExport(KPADGetMplsZeroPlayParam);

   RegisterFunctionExport(KPADIsEnableMplsZeroDrift);
   RegisterFunctionExport(KPADInitMplsZeroDriftMode);
   RegisterFunctionExport(KPADSetMplsZeroDriftMode);
   RegisterFunctionExport(KPADGetMplsZeroDriftMode);
   RegisterFunctionExport(KPADSetMplsMagnification);

   RegisterFunctionExport(KPADEnableStickCrossClamp);
   RegisterFunctionExport(KPADDisableStickCrossClamp);

   RegisterFunctionExport(KPADSetReviseMode);
   RegisterFunctionExport(KPADSetFSStickClamp);
   RegisterFunctionExport(KPADGetReviseAngle);

   RegisterDataInternal(sKpadData);
}

} // namespace cafe::padscore
