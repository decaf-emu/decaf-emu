#include "erreula.h"
#include "erreula_errorviewer.h"

namespace Rpl
{

void
ErrEulaAppearError(nn::erreula::AppearArg *args)
{
}

void
ErrEulaCalc(nn::erreula::ControllerInfo *info)
{
}

void
ErrEulaCreate(uint8_t *unk1, nn::erreula::RegionType region, nn::erreula::LangType language, FSClient *fsClient)
{
}

void
ErrEulaDestroy()
{
}

void
ErrEulaDisappearError()
{
}

void
ErrEulaDrawDRC()
{
}

void
ErrEulaDrawTV()
{
}

ErrorViewerState
ErrEulaGetStateErrorViewer()
{
   return ErrorViewerState::None;
}

BOOL
ErrEulaIsDecideSelectButtonError()
{
   return FALSE;
}

BOOL
ErrEulaIsDecideSelectLeftButtonError()
{
   return FALSE;
}

BOOL
ErrEulaIsDecideSelectRightButtonError()
{
   return FALSE;
}

void
ErrEulaSetControllerRemo(nn::erreula::ControllerType type)
{
}

void
ErrEulaAppearHomeNixSign(nn::erreula::HomeNixSignArg *arg)
{
}

BOOL
ErrEulaIsAppearHomeNixSign()
{
   return FALSE;
}

void
ErrEulaDisappearHomeNixSign()
{
}

void
ErrEulaChangeLang(nn::erreula::LangType language)
{
}

BOOL
ErrEulaIsSelectCursorActive()
{
   return FALSE;
}

nn::erreula::ResultType
ErrEulaGetResultType()
{
   return static_cast<nn::erreula::ResultType>(1);
}

uint32_t
ErrEulaGetResultCode()
{
   return 0;
}

uint32_t
ErrEulaGetSelectButtonNumError()
{
   return 0;
}

void
ErrEulaSetVersion(int version)
{
}

void
ErrEulaPlayAppearSE(bool value)
{
}

bool
ErrEulaJump(char const *, unsigned int)
{
   return false;
}

} // namespace Rpl

void ErrEula::registerErrorViewerFunctions()
{
   RegisterKernelFunctionName("ErrEulaAppearError__3RplFRCQ3_2nn7erreula9AppearArg", Rpl::ErrEulaAppearError);
   RegisterKernelFunctionName("ErrEulaCalc__3RplFRCQ3_2nn7erreula14ControllerInfo", Rpl::ErrEulaCalc);
   RegisterKernelFunctionName("ErrEulaCreate__3RplFPUcQ3_2nn7erreula10RegionTypeQ3_2nn7erreula8LangTypeP8FSClient", Rpl::ErrEulaCreate);
   RegisterKernelFunctionName("ErrEulaDestroy__3RplFv", Rpl::ErrEulaDestroy);
   RegisterKernelFunctionName("ErrEulaDisappearError__3RplFv", Rpl::ErrEulaDisappearError);
   RegisterKernelFunctionName("ErrEulaDrawDRC__3RplFv", Rpl::ErrEulaDrawDRC);
   RegisterKernelFunctionName("ErrEulaDrawTV__3RplFv", Rpl::ErrEulaDrawTV);
   RegisterKernelFunctionName("ErrEulaGetStateErrorViewer__3RplFv", Rpl::ErrEulaGetStateErrorViewer);
   RegisterKernelFunctionName("ErrEulaIsDecideSelectButtonError__3RplFv", Rpl::ErrEulaIsDecideSelectButtonError);
   RegisterKernelFunctionName("ErrEulaIsDecideSelectLeftButtonError__3RplFv", Rpl::ErrEulaIsDecideSelectLeftButtonError);
   RegisterKernelFunctionName("ErrEulaIsDecideSelectRightButtonError__3RplFv", Rpl::ErrEulaIsDecideSelectRightButtonError);
   RegisterKernelFunctionName("ErrEulaSetControllerRemo__3RplFQ3_2nn7erreula14ControllerType", Rpl::ErrEulaSetControllerRemo);
   RegisterKernelFunctionName("ErrEulaAppearHomeNixSign__3RplFRCQ3_2nn7erreula14HomeNixSignArg", Rpl::ErrEulaAppearHomeNixSign);
   RegisterKernelFunctionName("ErrEulaIsAppearHomeNixSign__3RplFv", Rpl::ErrEulaIsAppearHomeNixSign);
   RegisterKernelFunctionName("ErrEulaDisappearHomeNixSign__3RplFv", Rpl::ErrEulaDisappearHomeNixSign);
   RegisterKernelFunctionName("ErrEulaChangeLang__3RplFQ3_2nn7erreula8LangType", Rpl::ErrEulaChangeLang);
   RegisterKernelFunctionName("ErrEulaIsSelectCursorActive__3RplFv", Rpl::ErrEulaIsSelectCursorActive);
   RegisterKernelFunctionName("ErrEulaGetResultType__3RplFv", Rpl::ErrEulaGetResultType);
   RegisterKernelFunctionName("ErrEulaGetResultCode__3RplFv", Rpl::ErrEulaGetResultCode);
   RegisterKernelFunctionName("ErrEulaGetSelectButtonNumError__3RplFv", Rpl::ErrEulaGetSelectButtonNumError);
   RegisterKernelFunctionName("ErrEulaSetVersion__3RplFi", Rpl::ErrEulaSetVersion);
   RegisterKernelFunctionName("ErrEulaPlayAppearSE__3RplFb", Rpl::ErrEulaPlayAppearSE);
   RegisterKernelFunctionName("ErrEulaJump__3RplFPCcUi", Rpl::ErrEulaJump);
}
