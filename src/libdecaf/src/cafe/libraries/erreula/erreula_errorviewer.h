#pragma once
#include "erreula_enum.h"
#include "cafe/libraries/coreinit/coreinit_fs_client.h"
#include <libcpu/be2_struct.h>

namespace cafe::nn_erreula
{

struct AppearArg;
struct ControllerInfo;
struct HomeNixSignArg;

void
ErrEulaAppearError(virt_ptr<AppearArg> args);

void
ErrEulaCalc(virt_ptr<ControllerInfo> info);

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
ErrEulaAppearHomeNixSign(virt_ptr<HomeNixSignArg> arg);

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
ErrEulaJump(virt_ptr<char const> a1,
            uint32_t a2);

} // namespace cafe::nn_erreula
