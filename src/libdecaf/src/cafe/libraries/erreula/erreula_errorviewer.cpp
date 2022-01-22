#include "erreula.h"
#include "erreula_errorviewer.h"

#include "cafe/libraries/cafe_hle_stub.h"

#include "decaf_erreula.h"

#include <mutex>

namespace cafe::nn_erreula
{

struct StaticErrEulaData
{
   be2_val<ErrorViewerState> errorViewerState;
   be2_val<bool> buttonPressed;
   be2_val<bool> button2Pressed;
   be2_virt_ptr<uint8_t> workMemory;
};

static virt_ptr<StaticErrEulaData> sErrEulaData = nullptr;

// Uses a real mutex because we interact with outside world via ErrEulaDriver
static std::mutex sMutex;

void
ErrEulaAppearError(virt_ptr<const AppearArg> args)
{
   {
      std::unique_lock<std::mutex> lock { sMutex };
      sErrEulaData->errorViewerState = ErrorViewerState::Visible;
   }

   if (auto driver = decaf::errEulaDriver()) {
      switch (args->errorArg.errorType) {
      case ErrorType::Code:
         driver->onOpenErrorCode(args->errorArg.errorCode);
         break;
      case ErrorType::Message:
         driver->onOpenErrorMessage(args->errorArg.errorMessage.getRawPointer());
         break;
      case ErrorType::Message1Button:
         driver->onOpenErrorMessage(args->errorArg.errorMessage.getRawPointer(),
                                    args->errorArg.button1Label.getRawPointer());
         break;
      case ErrorType::Message2Button:
         driver->onOpenErrorMessage(args->errorArg.errorMessage.getRawPointer(),
                                    args->errorArg.button1Label.getRawPointer(),
                                    args->errorArg.button2Label.getRawPointer());
         break;
      }
   }
}

void
ErrEulaCalc(virt_ptr<const ControllerInfo> info)
{
}

BOOL
ErrEulaCreate(virt_ptr<uint8_t> workMemory,
              RegionType region,
              LangType language,
              virt_ptr<coreinit::FSClient> fsClient)
{
   return TRUE;
}

void
ErrEulaDestroy()
{
}

void
ErrEulaDisappearError()
{
   {
      std::unique_lock<std::mutex> lock { sMutex };
      sErrEulaData->errorViewerState = ErrorViewerState::Hidden;
   }

   if (auto driver = decaf::errEulaDriver()) {
      driver->onClose();
   }
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
   std::unique_lock<std::mutex> lock { sMutex };
   return sErrEulaData->errorViewerState;
}

BOOL
ErrEulaIsDecideSelectButtonError()
{
   std::unique_lock<std::mutex> lock { sMutex };
   return sErrEulaData->buttonPressed;
}

BOOL
ErrEulaIsDecideSelectLeftButtonError()
{
   std::unique_lock<std::mutex> lock{ sMutex };
   return sErrEulaData->buttonPressed;
}

BOOL
ErrEulaIsDecideSelectRightButtonError()
{
   std::unique_lock<std::mutex> lock{ sMutex };
   return sErrEulaData->button2Pressed;
}

void
ErrEulaSetControllerRemo(ControllerType type)
{
   decaf_warn_stub();
}

void
ErrEulaAppearHomeNixSign(virt_ptr<const HomeNixSignArg> arg)
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
ErrEulaJump(virt_ptr<const char> a1,
            uint32_t a2)
{
   decaf_warn_stub();
   return false;
}

namespace internal
{

void
buttonClicked()
{
   std::unique_lock<std::mutex> lock { sMutex };
   if (sErrEulaData->errorViewerState != ErrorViewerState::Visible) {
      return;
   }

   sErrEulaData->buttonPressed = true;
}

void
button1Clicked()
{
   std::unique_lock<std::mutex> lock { sMutex };
   if (sErrEulaData->errorViewerState != ErrorViewerState::Visible) {
      return;
   }

   sErrEulaData->buttonPressed = true;
}

void
button2Clicked()
{
   std::unique_lock<std::mutex> lock{ sMutex };
   if (sErrEulaData->errorViewerState != ErrorViewerState::Visible) {
      return;
   }

   sErrEulaData->button2Pressed = true;
}

} // internal

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

   RegisterDataInternal(sErrEulaData);
}

} // namespace cafe::nn_erreula
