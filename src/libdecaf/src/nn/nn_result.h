#pragma once
#include <cstdint>

namespace nn
{

struct Result
{
   enum Level : int32_t
   {
      LEVEL_SUCCESS  = 0,
      LEVEL_FATAL    = -1,
      LEVEL_USAGE    = -2,
      LEVEL_STATUS   = -3,
      LEVEL_END      = -7,
   };

   enum Module : int32_t
   {
      MODULE_COMMON        = 0,
      MODULE_NN_IPC        = 1,
      MODULE_NN_BOSS       = 2,
      MODULE_NN_ACP        = 3,
      MODULE_NN_IOS        = 4,
      MODULE_NN_NIM        = 5,
      MODULE_NN_PDM        = 6,
      MODULE_NN_ACT        = 7,
      MODULE_NN_NGC        = 8,
      MODULE_NN_ECA        = 9,
      MODULE_NN_NUP        = 10,
      MODULE_NN_NDM        = 11,
      MODULE_NN_FP         = 12,
      MODULE_NN_AC         = 13,
      MODULE_NN_CONNTEST   = 14,
      MODULE_NN_DRMAPP     = 15,
      MODULE_NN_TELNET     = 16,
      MODULE_NN_OLV        = 17,
      MODULE_NN_VCTL       = 18,
      MODULE_NN_NEIA       = 19,
      MODULE_NN_SPM        = 20,
      MODULE_NN_EMD        = 21,
      MODULE_NN_EC         = 22,
      MODULE_NN_CIA        = 23,
      MODULE_NN_SL         = 24,
      MODULE_NN_ECO        = 25,
      MODULE_NN_TRIAL      = 26,
      MODULE_NN_NFP        = 27,
      MODULE_NN_TEST       = 125,
   };

   enum LegacyLevel
   {
      LEGACY_LEVEL_INFO                   = 1,
      LEGACY_LEVEL_RESET                  = -4,
      LEGACY_LEVEL_REINIT                 = -5,
      LEGACY_LEVEL_PERMANENT              = -6,
      LEGACY_LEVEL_TEMPORARY              = -7,
   };

   enum LegacyCommonDescription : int32_t
   {
      LEGACY_DESCRIPTION_SUCCESS                 = 0,
      LEGACY_DESCRIPTION_TIMEOUT                 = -2,
      LEGACY_DESCRIPTION_OUT_OF_RANGE            = -3,
      LEGACY_DESCRIPTION_ALREADY_EXISTS          = -4,
      LEGACY_DESCRIPTION_CANCEL_REQUESTED        = -5,
      LEGACY_DESCRIPTION_NOT_FOUND               = -6,
      LEGACY_DESCRIPTION_ALREADY_INITIALIZED     = -7,
      LEGACY_DESCRIPTION_NOT_INITIALIZED         = -8,
      LEGACY_DESCRIPTION_INVALID_HANDLE          = -9,
      LEGACY_DESCRIPTION_INVALID_POINTER         = -10,
      LEGACY_DESCRIPTION_INVALID_ADDRESS         = -11,
      LEGACY_DESCRIPTION_NOT_IMPLEMENTED         = -12,
      LEGACY_DESCRIPTION_OUT_OF_MEMORY           = -13,
      LEGACY_DESCRIPTION_MISALIGNED_SIZE         = -14,
      LEGACY_DESCRIPTION_MISALIGNED_ADDRESS      = -15,
      LEGACY_DESCRIPTION_BUSY                    = -16,
      LEGACY_DESCRIPTION_NO_DATA                 = -17,
      LEGACY_DESCRIPTION_INVALID_COMBINATION     = -18,
      LEGACY_DESCRIPTION_INVALID_ENUM_VALUE      = -19,
      LEGACY_DESCRIPTION_INVALID_SIZE            = -20,
      LEGACY_DESCRIPTION_ALREADY_DONE            = -21,
      LEGACY_DESCRIPTION_NOT_AUTHORIZED          = -22,
      LEGACY_DESCRIPTION_TOO_LARGE               = -23,
      LEGACY_DESCRIPTION_INVALID_SELECTION       = -24,
   };

   enum LegacyModule
   {
      LEGACY_MODULE_COMMON                = 0,
      LEGACY_MODULE_NN_KERNEL             = 1,
      LEGACY_MODULE_NN_UTIL               = 2,
      LEGACY_MODULE_NN_FILE_SERVER        = 3,
      LEGACY_MODULE_NN_LOADER_SERVER      = 4,
      LEGACY_MODULE_NN_TCB                = 5,
      LEGACY_MODULE_NN_OS                 = 6,
      LEGACY_MODULE_NN_DBG                = 7,
      LEGACY_MODULE_NN_DMNT               = 8,
      LEGACY_MODULE_NN_PDN                = 9,
      LEGACY_MODULE_NN_GX                 = 0xA,
      LEGACY_MODULE_NN_I2C                = 0xB,
      LEGACY_MODULE_NN_GPIO               = 0xC,
      LEGACY_MODULE_NN_DD                 = 0xD,
      LEGACY_MODULE_NN_CODEC              = 0xE,
      LEGACY_MODULE_NN_SPI                = 0xF,
      LEGACY_MODULE_NN_PXI                = 0x10,
      LEGACY_MODULE_NN_FS                 = 0x11,
      LEGACY_MODULE_NN_DI                 = 0x12,
      LEGACY_MODULE_NN_HID                = 0x13,
      LEGACY_MODULE_NN_CAMERA             = 0x14,
      LEGACY_MODULE_NN_PI                 = 0x15,
      LEGACY_MODULE_NN_PM                 = 0x16,
      LEGACY_MODULE_NN_PMLOW              = 0x17,
      LEGACY_MODULE_NN_FSI                = 0x18,
      LEGACY_MODULE_NN_SRV                = 0x19,
      LEGACY_MODULE_NN_NDM                = 0x1A,
      LEGACY_MODULE_NN_NWM                = 0x1B,
      LEGACY_MODULE_NN_SOCKET             = 0x1C,
      LEGACY_MODULE_NN_LDR                = 0x1D,
      LEGACY_MODULE_NN_ACC                = 0x1E,
      LEGACY_MODULE_NN_ROMFS              = 0x1F,
      LEGACY_MODULE_NN_AM                 = 0x20,
      LEGACY_MODULE_NN_HIO                = 0x21,
      LEGACY_MODULE_NN_UPDATER            = 0x22,
      LEGACY_MODULE_NN_MIC                = 0x23,
      LEGACY_MODULE_NN_FND                = 0x24,
      LEGACY_MODULE_NN_MP                 = 0x25,
      LEGACY_MODULE_NN_MPWL               = 0x26,
      LEGACY_MODULE_NN_AC                 = 0x27,
      LEGACY_MODULE_NN_HTTP               = 0x28,
      LEGACY_MODULE_NN_DSP                = 0x29,
      LEGACY_MODULE_NN_SND                = 0x2A,
      LEGACY_MODULE_NN_DLP                = 0x2B,
      LEGACY_MODULE_NN_HIOLOW             = 0x2C,
      LEGACY_MODULE_NN_CSND               = 0x2D,
      LEGACY_MODULE_NN_SSL                = 0x2E,
      LEGACY_MODULE_NN_AMLOW              = 0x2F,
      LEGACY_MODULE_NN_NEX                = 0x30,
      LEGACY_MODULE_NN_FRIENDS            = 0x31,
      LEGACY_MODULE_NN_RDT                = 0x32,
      LEGACY_MODULE_NN_APPLET             = 0x33,
      LEGACY_MODULE_NN_NIM                = 0x34,
      LEGACY_MODULE_NN_PTM                = 0x35,
      LEGACY_MODULE_NN_MIDI               = 0x36,
      LEGACY_MODULE_NN_MC                 = 0x37,
      LEGACY_MODULE_NN_SWC                = 0x38,
      LEGACY_MODULE_NN_FATFS              = 0x39,
      LEGACY_MODULE_NN_NGC                = 0x3A,
      LEGACY_MODULE_NN_CARD               = 0x3B,
      LEGACY_MODULE_NN_CARDNOR            = 0x3C,
      LEGACY_MODULE_NN_SDMC               = 0x3D,
      LEGACY_MODULE_NN_BOSS               = 0x3E,
      LEGACY_MODULE_NN_DBM                = 0x3F,
      LEGACY_MODULE_NN_CFG                = 0x40,
      LEGACY_MODULE_NN_PS                 = 0x41,
      LEGACY_MODULE_NN_CEC                = 0x42,
      LEGACY_MODULE_NN_IR                 = 0x43,
      LEGACY_MODULE_NN_UDS                = 0x44,
      LEGACY_MODULE_NN_PL                 = 0x45,
      LEGACY_MODULE_NN_CUP                = 0x46,
      LEGACY_MODULE_NN_GYROSCOPE          = 0x47,
      LEGACY_MODULE_NN_MCU                = 0x48,
      LEGACY_MODULE_NN_NS                 = 0x49,
      LEGACY_MODULE_NN_NEWS               = 0x4A,
      LEGACY_MODULE_NN_RO                 = 0x4B,
      LEGACY_MODULE_NN_GD                 = 0x4C,
      LEGACY_MODULE_NN_CARDSPI            = 0x4D,
      LEGACY_MODULE_NN_EC                 = 0x4E,
      LEGACY_MODULE_NN_WEBBRS             = 0x4F,
      LEGACY_MODULE_NN_TEST               = 0x50,
      LEGACY_MODULE_NN_ENC                = 0x51,
      LEGACY_MODULE_NN_PIA                = 0x52,
      LEGACY_MODULE_APPLICATION           = 0x1FE,
   };

   enum LegacySummary
   {
      LEGACY_SUMMARY_SUCCESS              = 0,
      LEGACY_SUMMARY_NOTHING_HAPPENED     = 1,
      LEGACY_SUMMARY_WOULD_BLOCK          = 2,
      LEGACY_SUMMARY_OUT_OF_RESOURCE      = 3,
      LEGACY_SUMMARY_NOT_FOUND            = 4,
      LEGACY_SUMMARY_INVALID_STATE        = 5,
      LEGACY_SUMMARY_NOT_SUPPORTED        = 6,
      LEGACY_SUMMARY_INVALID_ARGUMENT     = 7,
      LEGACY_SUMMARY_WRONG_ARGUMENT       = 8,
      LEGACY_SUMMARY_CANCELLED            = 9,
      LEGACY_SUMMARY_STATUS_CHANGED       = 10,
      LEGACY_SUMMARY_INTERNAL             = 11,
   };

   enum LegacySignature
   {
      LEGACY_SIGNATURE                    = 3,
   };

   constexpr Result() noexcept :
      mCode(0)
   {
   }

   constexpr Result(uint32_t code_) noexcept :
      mCode(code_)
   {
   }

   constexpr Result(Module module, Level level, int32_t description) noexcept :
      mDescription(description),
      mModule(module),
      mLevel(level)
   {
   }

   constexpr explicit operator uint32_t() const noexcept
   {
      return mCode;
   }

   constexpr explicit operator bool() const noexcept
   {
      return ok();
   }

   constexpr bool operator==(const Result &other) const noexcept
   {
      return module() == other.module() &&
             description() == other.description();
   }

   constexpr bool ok() const noexcept
   {
      return (mCode & 0x80000000) == 0;
   }

   constexpr bool failed() const noexcept
   {
      return (mCode & 0x80000000) != 0;
   }

   constexpr bool isLegacyRepresentation() const noexcept
   {
      return mLegacy.signature == LEGACY_SIGNATURE;
   }

   constexpr uint32_t code() const noexcept
   {
      return mCode;
   }

   constexpr int description() const noexcept
   {
      if (isLegacyRepresentation()) {
         return mLegacy.description;
      }

      return mDescription;
   }

   constexpr int level() const noexcept
   {
      if (isLegacyRepresentation()) {
         return mLegacy.level;
      }

      return mLevel;
   }

   constexpr int module() const noexcept
   {
      if (isLegacyRepresentation()) {
         return mLegacy.module;
      }

      return mModule;
   }

   constexpr int legacySummary() const noexcept
   {
      return mLegacy.summary;
   }

private:
   union
   {
      uint32_t mCode;

      struct
      {
         int32_t mDescription : 20;
         int32_t mModule : 9;
         int32_t mLevel : 3;
      };

      struct
      {
         int32_t description : 10;
         int32_t summary : 4;
         int32_t level : 4;
         int32_t : 2;
         int32_t module : 7;
         int32_t signature : 2;
         int32_t : 3;
      } mLegacy;
   };
};

static constexpr Result ResultSuccess { 0 };

template<Result::Module Module_, Result::Level Level_, int DescStart, int DescEnd>
struct ResultRange : Result
{
   constexpr ResultRange() :
      Result(Module_, Level_, DescStart)
   {
   }

   constexpr bool operator==(const Result &other) noexcept
   {
      return other.module() == Module_ &&
             other.description() >= DescStart &&
             other.description() < DescEnd;
   }

   constexpr friend bool operator==(const Result &other, const ResultRange &) noexcept
   {
      return other.module() == Module_ &&
             other.description() >= DescStart &&
             other.description() < DescEnd;
   }
};

} // namespace nn
