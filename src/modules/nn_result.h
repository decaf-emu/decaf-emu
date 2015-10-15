#pragma once
#include <gsl.h>
#include "types.h"
#include "ppctypes.h"

namespace nn
{

union Result
{
   enum Level : int32_t // nn::dbg::result::GetLevelString
   {
      LEVEL_SUCCESS = 0,
      LEVEL_FATAL = -1,
      LEVEL_USAGE = -2,
      LEVEL_STATUS = -3,
      LEVEL_END = -7,
   };

   enum Module : int32_t // nn::dbg::result::GetModuleString
   {
      MODULE_COMMON = 0,
      MODULE_NN_IPC = 1,
      MODULE_NN_BOSS = 2,
      MODULE_NN_ACP = 3,
      MODULE_NN_IOS = 4,
      MODULE_NN_NIM = 5,
      MODULE_NN_PDM = 6,
      MODULE_NN_ACT = 7,
      MODULE_NN_NGC = 8,
      MODULE_NN_ECA = 9,
      MODULE_NN_NUP = 10,
      MODULE_NN_NDM = 11,
      MODULE_NN_FP = 12,
      MODULE_NN_AC = 13,
      MODULE_NN_CONNTEST = 14,
      MODULE_NN_DRMAPP = 15,
      MODULE_NN_TELNET = 16,
      MODULE_NN_OLV = 17,
      MODULE_NN_VCTL = 18,
      MODULE_NN_NEIA = 19,
      MODULE_NN_SPM = 20,
      MODULE_NN_EMD = 21,
      MODULE_NN_EC = 22,
      MODULE_NN_CIA = 23,
      MODULE_NN_SL = 24,
      MODULE_NN_ECO = 25,
      MODULE_NN_TRIAL = 26,
      MODULE_NN_NFP = 27,
      MODULE_NN_TEST = 125,
   };

   enum Description : int32_t // nn::dbg::result::GetDescriptionString
   {
   };

   static const Result Success;

   Result() :
      code(0)
   {
   }

   Result(uint32_t code) :
      code(code)
   {
   }

   Result(int32_t module, int32_t level, int32_t description) :
      module(module),
      level(level),
      description(description)
   {
   }

   struct
   {
      int32_t description : 20;
      int32_t module : 9;
      int32_t level : 3;
   };

   uint32_t code;
};

} // namespace nn

namespace ppctypes
{

template<>
struct ppctype_converter_t<nn::Result>
{
   typedef nn::Result Type;
   static const PpcType ppc_type = PpcType::WORD;

   static inline void to_ppc(const Type& v, uint32_t& out)
   {
      out = v.code;
   }

   static inline Type from_ppc(uint32_t in)
   {
      return nn::Result { in };
   }
};

} // namespace ppctypes
