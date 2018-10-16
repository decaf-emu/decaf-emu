#include "erreula.h"
#include "erreula_errorviewer.h"

#include "cafe/libraries/cafe_hle_stub.h"

namespace cafe::nn_erreula
{

void
ErrEulaAppearError(virt_ptr<AppearArg> args)
{
   decaf_warn_stub();
}

void
ErrEulaCalc(virt_ptr<ControllerInfo> info)
{
   decaf_warn_stub();
}

BOOL
ErrEulaCreate(virt_ptr<uint8_t> workMemory,
              RegionType region,
              LangType language,
              virt_ptr<coreinit::FSClient> fsClient)
{
   decaf_warn_stub();
   return TRUE;
}

void
ErrEulaDestroy()
{
   decaf_warn_stub();
}

void
ErrEulaDisappearError()
{
   decaf_warn_stub();
}

void
ErrEulaDrawDRC()
{
   decaf_warn_stub();
}

void
ErrEulaDrawTV()
{
   decaf_warn_stub();
}

ErrorViewerState
ErrEulaGetStateErrorViewer()
{
   decaf_warn_stub();
   return ErrorViewerState::None;
}

BOOL
ErrEulaIsDecideSelectButtonError()
{
   decaf_warn_stub();
   return FALSE;
}

BOOL
ErrEulaIsDecideSelectLeftButtonError()
{
   decaf_warn_stub();
   return FALSE;
}

BOOL
ErrEulaIsDecideSelectRightButtonError()
{
   decaf_warn_stub();
   return FALSE;
}

void
ErrEulaSetControllerRemo(ControllerType type)
{
   decaf_warn_stub();
}

void
ErrEulaAppearHomeNixSign(virt_ptr<HomeNixSignArg> arg)
{
   decaf_warn_stub();
}

BOOL
ErrEulaIsAppearHomeNixSign()
{
   decaf_warn_stub();
   return FALSE;
}

void
ErrEulaDisappearHomeNixSign()
{
   decaf_warn_stub();
}

void
ErrEulaChangeLang(LangType language)
{
   decaf_warn_stub();
}

BOOL
ErrEulaIsSelectCursorActive()
{
   decaf_warn_stub();
   return FALSE;
}

ResultType
ErrEulaGetResultType()
{
   decaf_warn_stub();
   return static_cast<ResultType>(1);
}

uint32_t
ErrEulaGetResultCode()
{
   decaf_warn_stub();
   return 0;
}

uint32_t
ErrEulaGetSelectButtonNumError()
{
   decaf_warn_stub();
   return 0;
}

void
ErrEulaSetVersion(int32_t version)
{
   decaf_warn_stub();
}

void
ErrEulaPlayAppearSE(bool value)
{
   decaf_warn_stub();
}

bool
ErrEulaJump(virt_ptr<char const> a1,
            uint32_t a2)
{
   decaf_warn_stub();
   return false;
}

void Library::registerErrorViewerSymbols()
{
   RegisterFunctionExportName("ErrEulaAppearError__3RplFRCQ3_2nn7erreula9AppearArg",
                              ErrEulaAppearError);
   RegisterFunctionExportName("ErrEulaCalc__3RplFRCQ3_2nn7erreula14ControllerInfo",
                              ErrEulaCalc);
   RegisterFunctionExportName("ErrEulaCreate__3RplFPUcQ3_2nn7erreula10RegionTypeQ3_2nn7erreula8LangTypeP8FSClient",
                              ErrEulaCreate);
   RegisterFunctionExportName("ErrEulaDestroy__3RplFv",
                              ErrEulaDestroy);
   RegisterFunctionExportName("ErrEulaDisappearError__3RplFv",
                              ErrEulaDisappearError);
   RegisterFunctionExportName("ErrEulaDrawDRC__3RplFv",
                              ErrEulaDrawDRC);
   RegisterFunctionExportName("ErrEulaDrawTV__3RplFv",
                              ErrEulaDrawTV);
   RegisterFunctionExportName("ErrEulaGetStateErrorViewer__3RplFv",
                              ErrEulaGetStateErrorViewer);
   RegisterFunctionExportName("ErrEulaIsDecideSelectButtonError__3RplFv",
                              ErrEulaIsDecideSelectButtonError);
   RegisterFunctionExportName("ErrEulaIsDecideSelectLeftButtonError__3RplFv",
                              ErrEulaIsDecideSelectLeftButtonError);
   RegisterFunctionExportName("ErrEulaIsDecideSelectRightButtonError__3RplFv",
                              ErrEulaIsDecideSelectRightButtonError);
   RegisterFunctionExportName("ErrEulaSetControllerRemo__3RplFQ3_2nn7erreula14ControllerType",
                              ErrEulaSetControllerRemo);
   RegisterFunctionExportName("ErrEulaAppearHomeNixSign__3RplFRCQ3_2nn7erreula14HomeNixSignArg",
                              ErrEulaAppearHomeNixSign);
   RegisterFunctionExportName("ErrEulaIsAppearHomeNixSign__3RplFv",
                              ErrEulaIsAppearHomeNixSign);
   RegisterFunctionExportName("ErrEulaDisappearHomeNixSign__3RplFv",
                              ErrEulaDisappearHomeNixSign);
   RegisterFunctionExportName("ErrEulaChangeLang__3RplFQ3_2nn7erreula8LangType",
                              ErrEulaChangeLang);
   RegisterFunctionExportName("ErrEulaIsSelectCursorActive__3RplFv",
                              ErrEulaIsSelectCursorActive);
   RegisterFunctionExportName("ErrEulaGetResultType__3RplFv",
                              ErrEulaGetResultType);
   RegisterFunctionExportName("ErrEulaGetResultCode__3RplFv",
                              ErrEulaGetResultCode);
   RegisterFunctionExportName("ErrEulaGetSelectButtonNumError__3RplFv",
                              ErrEulaGetSelectButtonNumError);
   RegisterFunctionExportName("ErrEulaSetVersion__3RplFi",
                              ErrEulaSetVersion);
   RegisterFunctionExportName("ErrEulaPlayAppearSE__3RplFb",
                              ErrEulaPlayAppearSE);
   RegisterFunctionExportName("ErrEulaJump__3RplFPCcUi",
                              ErrEulaJump);
}

} // namespace cafe::nn_erreula
