#pragma once
#include "erreula_enum.h"

#include <libcpu/be2_struct.h>

namespace cafe::coreinit
{
struct FSClient;
} // namespace cafe::coreinit

namespace cafe::kpad
{
struct KPADStatus;
} // namespace cafe::kpad

namespace cafe::vpad
{
struct VPADStatus;
} // namespace cafe::vpad

namespace cafe::nn_erreula
{

struct ErrorArg
{
   be2_val<ErrorType> errorType;
   be2_val<RenderTarget> renderTarget;
   be2_val<ControllerType> controllerType;
   be2_val<uint32_t> unknown0x0C;
   be2_val<int32_t> errorCode;
   be2_val<uint32_t> unknown0x14;
   be2_virt_ptr<const char16_t> errorMessage;
   be2_virt_ptr<const char16_t> button1Label;
   be2_virt_ptr<const char16_t> button2Label;
   be2_virt_ptr<const char16_t> errorTitle;
   be2_val<bool> unknown0x28;
   PADDING(3);
};
CHECK_OFFSET(ErrorArg, 0x00, errorType);
CHECK_OFFSET(ErrorArg, 0x04, renderTarget);
CHECK_OFFSET(ErrorArg, 0x08, controllerType);
CHECK_OFFSET(ErrorArg, 0x0C, unknown0x0C);
CHECK_OFFSET(ErrorArg, 0x10, errorCode);
CHECK_OFFSET(ErrorArg, 0x14, unknown0x14);
CHECK_OFFSET(ErrorArg, 0x18, errorMessage);
CHECK_OFFSET(ErrorArg, 0x1C, button1Label);
CHECK_OFFSET(ErrorArg, 0x20, button2Label);
CHECK_OFFSET(ErrorArg, 0x24, errorTitle);
CHECK_OFFSET(ErrorArg, 0x28, unknown0x28);
CHECK_SIZE(ErrorArg, 0x2C);

struct AppearArg
{
   be2_struct<ErrorArg> errorArg;
};
CHECK_OFFSET(AppearArg, 0x00, errorArg);
CHECK_SIZE(AppearArg, 0x2C);

struct ControllerInfo
{
   be2_virt_ptr<vpad::VPADStatus> vpad;
   be2_array<virt_ptr<kpad::KPADStatus>, 4> kpad;
};
CHECK_OFFSET(ControllerInfo, 0x00, vpad);
CHECK_OFFSET(ControllerInfo, 0x04, kpad);
CHECK_SIZE(ControllerInfo, 0x14);

struct HomeNixSignArg
{
   be2_val<uint32_t> unknown0x00;
};
CHECK_OFFSET(HomeNixSignArg, 0x00, unknown0x00);
CHECK_SIZE(HomeNixSignArg, 0x04);

void
ErrEulaAppearError(virt_ptr<const AppearArg> args);

void
ErrEulaCalc(virt_ptr<const ControllerInfo> info);

BOOL
ErrEulaCreate(virt_ptr<uint8_t> workMemory,
              RegionType region,
              LangType language,
              virt_ptr<coreinit::FSClient> fsClient);

void
ErrEulaDestroy();

void
ErrEulaDisappearError();

void
ErrEulaDrawDRC();

void
ErrEulaDrawTV();

ErrorViewerState
ErrEulaGetStateErrorViewer();

BOOL
ErrEulaIsDecideSelectButtonError();

BOOL
ErrEulaIsDecideSelectLeftButtonError();

BOOL
ErrEulaIsDecideSelectRightButtonError();

void
ErrEulaSetControllerRemo(ControllerType type);

void
ErrEulaAppearHomeNixSign(virt_ptr<const HomeNixSignArg> arg);

BOOL
ErrEulaIsAppearHomeNixSign();

void
ErrEulaDisappearHomeNixSign();

void
ErrEulaChangeLang(LangType language);

BOOL
ErrEulaIsSelectCursorActive();

ResultType
ErrEulaGetResultType();

uint32_t
ErrEulaGetResultCode();

uint32_t
ErrEulaGetSelectButtonNumError();

void
ErrEulaSetVersion(int32_t version);

void
ErrEulaPlayAppearSE(bool value);

bool
ErrEulaJump(virt_ptr<const char> a1,
            uint32_t a2);

namespace internal
{

void
buttonClicked();

void
button1Clicked();

void
button2Clicked();

} // internal

} // namespace cafe::nn_erreula
