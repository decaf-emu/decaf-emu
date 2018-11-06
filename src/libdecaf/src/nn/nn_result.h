#pragma once
#include <cstdint>

namespace nn
{

union Result
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

   enum Description : int32_t
   {
      DESCRIPTION_SUCCESS                 = 0,
      DESCRIPTION_TIMEOUT                 = -2,
      DESCRIPTION_OUT_OF_RANGE            = -3,
      DESCRIPTION_ALREADY_EXISTS          = -4,
      DESCRIPTION_CANCEL_REQUESTED        = -5,
      DESCRIPTION_NOT_FOUND               = -6,
      DESCRIPTION_ALREADY_INITIALIZED     = -7,
      DESCRIPTION_NOT_INITIALIZED         = -8,
      DESCRIPTION_INVALID_HANDLE          = -9,
      DESCRIPTION_INVALID_POINTER         = -10,
      DESCRIPTION_INVALID_ADDRESS         = -11,
      DESCRIPTION_NOT_IMPLEMENTED         = -12,
      DESCRIPTION_OUT_OF_MEMORY           = -13,
      DESCRIPTION_MISALIGNED_SIZE         = -14,
      DESCRIPTION_MISALIGNED_ADDRESS      = -15,
      DESCRIPTION_BUSY                    = -16,
      DESCRIPTION_NO_DATA                 = -17,
      DESCRIPTION_INVALID_COMBINATION     = -18,
      DESCRIPTION_INVALID_ENUM_VALUE      = -19,
      DESCRIPTION_INVALID_SIZE            = -20,
      DESCRIPTION_ALREADY_DONE            = -21,
      DESCRIPTION_NOT_AUTHORIZED          = -22,
      DESCRIPTION_TOO_LARGE               = -23,
      DESCRIPTION_INVALID_SELECTION       = -24,
   };

   constexpr Result() :
      code(0)
   {
   }

   constexpr Result(uint32_t code_) :
      code(code_)
   {
   }

   constexpr Result(int32_t module_, int32_t level_, int32_t description_) :
      description(description_),
      module(module_),
      level(level_)
   {
   }

   explicit operator uint32_t() const
   {
      return code;
   }

   bool ok() const
   {
      return static_cast<int32_t>(code) >= 0;
   }

   bool failed() const
   {
      return static_cast<int32_t>(code) < 0;
   }

   struct
   {
      int32_t description : 20;
      int32_t module : 9;
      int32_t level : 3;
   };

   uint32_t code;
};

static constexpr Result ResultSuccess { 0 };

} // namespace nn
