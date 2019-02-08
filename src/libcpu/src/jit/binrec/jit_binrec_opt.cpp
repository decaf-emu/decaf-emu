#include "jit_binrec.h"

#include <common/log.h>
#include <map>
#include <string>
#include <vector>

namespace cpu
{

namespace jit
{

struct OptFlagInfo
{
   enum {
      OPTFLAG_COMMON,
      OPTFLAG_GUEST,
      OPTFLAG_HOST,
      OPTFLAG_CHAIN
   } type;

   unsigned int value;
};

static const std::map<std::string, OptFlagInfo>
sOptFlags = {
   {"BASIC",                    {OptFlagInfo::OPTFLAG_COMMON,
                                 binrec::Optimize::BASIC}},
   {"DECONDITION",              {OptFlagInfo::OPTFLAG_COMMON,
                                 binrec::Optimize::DECONDITION}},
   {"DEEP_DATA_FLOW",           {OptFlagInfo::OPTFLAG_COMMON,
                                 binrec::Optimize::DEEP_DATA_FLOW}},
   {"DSE",                      {OptFlagInfo::OPTFLAG_COMMON,
                                 binrec::Optimize::DSE}},
   {"DSE_FP",                   {OptFlagInfo::OPTFLAG_COMMON,
                                 binrec::Optimize::DSE_FP}},
   {"FOLD_CONSTANTS",           {OptFlagInfo::OPTFLAG_COMMON,
                                 binrec::Optimize::FOLD_CONSTANTS}},
   {"FOLD_FP_CONSTANTS",        {OptFlagInfo::OPTFLAG_COMMON,
                                 binrec::Optimize::FOLD_FP_CONSTANTS}},
   {"NATIVE_IEEE_NAN",          {OptFlagInfo::OPTFLAG_COMMON,
                                 binrec::Optimize::NATIVE_IEEE_NAN}},
   {"NATIVE_IEEE_UNDERFLOW",    {OptFlagInfo::OPTFLAG_COMMON,
                                 binrec::Optimize::NATIVE_IEEE_UNDERFLOW}},

   {"PPC_ASSUME_NO_SNAN",       {OptFlagInfo::OPTFLAG_GUEST,
                                 binrec::Optimize::GuestPPC::ASSUME_NO_SNAN}},
   {"PPC_CONSTANT_GQRS",        {OptFlagInfo::OPTFLAG_GUEST,
                                 binrec::Optimize::GuestPPC::CONSTANT_GQRS}},
   {"PPC_DETECT_FCFI_EMUL",     {OptFlagInfo::OPTFLAG_GUEST,
                                 binrec::Optimize::GuestPPC::DETECT_FCFI_EMUL}},
   {"PPC_FAST_FCTIW",           {OptFlagInfo::OPTFLAG_GUEST,
                                 binrec::Optimize::GuestPPC::FAST_FCTIW}},
   {"PPC_FAST_FMADDS",          {OptFlagInfo::OPTFLAG_GUEST,
                                 binrec::Optimize::GuestPPC::FAST_FMADDS}},
   {"PPC_FAST_FMULS",           {OptFlagInfo::OPTFLAG_GUEST,
                                 binrec::Optimize::GuestPPC::FAST_FMULS}},
   {"PPC_FAST_STFS",            {OptFlagInfo::OPTFLAG_GUEST,
                                 binrec::Optimize::GuestPPC::FAST_STFS}},
   {"PPC_FNMADD_ZERO_SIGN",     {OptFlagInfo::OPTFLAG_GUEST,
                                 binrec::Optimize::GuestPPC::FNMADD_ZERO_SIGN}},
   {"PPC_FORWARD_LOADS",        {OptFlagInfo::OPTFLAG_GUEST,
                                 binrec::Optimize::GuestPPC::FORWARD_LOADS}},
   {"PPC_IGNORE_FPSCR_VXFOO",   {OptFlagInfo::OPTFLAG_GUEST,
                                 binrec::Optimize::GuestPPC::IGNORE_FPSCR_VXFOO}},
   {"PPC_NATIVE_RECIPROCAL",    {OptFlagInfo::OPTFLAG_GUEST,
                                 binrec::Optimize::GuestPPC::NATIVE_RECIPROCAL}},
   {"PPC_NO_FPSCR_STATE",       {OptFlagInfo::OPTFLAG_GUEST,
                                 binrec::Optimize::GuestPPC::NO_FPSCR_STATE}},
   {"PPC_PAIRED_LWARX_STWCX",   {OptFlagInfo::OPTFLAG_GUEST,
                                 binrec::Optimize::GuestPPC::PAIRED_LWARX_STWCX}},
   {"PPC_PS_STORE_DENORMALS",   {OptFlagInfo::OPTFLAG_GUEST,
                                 binrec::Optimize::GuestPPC::PS_STORE_DENORMALS}},
   {"PPC_SC_BLR",               {OptFlagInfo::OPTFLAG_GUEST,
                                 binrec::Optimize::GuestPPC::SC_BLR}},
   {"PPC_SINGLE_PREC_INPUTS",   {OptFlagInfo::OPTFLAG_GUEST,
                                 binrec::Optimize::GuestPPC::SINGLE_PREC_INPUTS}},
   {"PPC_TRIM_CR_STORES",       {OptFlagInfo::OPTFLAG_GUEST,
                                 binrec::Optimize::GuestPPC::TRIM_CR_STORES}},
   {"PPC_USE_SPLIT_FIELDS",     {OptFlagInfo::OPTFLAG_GUEST,
                                 binrec::Optimize::GuestPPC::USE_SPLIT_FIELDS}},

   {"X86_ADDRESS_OPERANDS",     {OptFlagInfo::OPTFLAG_HOST,
                                 binrec::Optimize::HostX86::ADDRESS_OPERANDS}},
   {"X86_BRANCH_ALIGNMENT",     {OptFlagInfo::OPTFLAG_HOST,
                                 binrec::Optimize::HostX86::BRANCH_ALIGNMENT}},
   {"X86_CONDITION_CODES",      {OptFlagInfo::OPTFLAG_HOST,
                                 binrec::Optimize::HostX86::CONDITION_CODES}},
   {"X86_FIXED_REGS",           {OptFlagInfo::OPTFLAG_HOST,
                                 binrec::Optimize::HostX86::FIXED_REGS}},
   {"X86_FORWARD_CONDITIONS",   {OptFlagInfo::OPTFLAG_HOST,
                                 binrec::Optimize::HostX86::FORWARD_CONDITIONS}},
   {"X86_MERGE_REGS",           {OptFlagInfo::OPTFLAG_HOST,
                                 binrec::Optimize::HostX86::MERGE_REGS}},
   {"X86_STORE_IMMEDIATE",      {OptFlagInfo::OPTFLAG_HOST,
                                 binrec::Optimize::HostX86::STORE_IMMEDIATE}},

   // Maps to sUseChaining instead of a flag value
   {"CHAIN",                    {OptFlagInfo::OPTFLAG_CHAIN}},
};

void
BinrecBackend::setOptFlags(const std::vector<std::string> &optList)
{
   mOptFlags.common = 0;
   mOptFlags.guest = 0;
   mOptFlags.host = 0;
   mOptFlags.useChaining = false;

   for (const auto &i : optList) {
      auto flag = sOptFlags.find(i);

      if (flag == sOptFlags.end()) {
         gLog->warn("Unknown optimization flag: {}", i);
         continue;
      }

      switch (flag->second.type) {
      case OptFlagInfo::OPTFLAG_CHAIN:
         mOptFlags.useChaining = true;
         break;
      case OptFlagInfo::OPTFLAG_COMMON:
         mOptFlags.common |= flag->second.value;
         break;
      case OptFlagInfo::OPTFLAG_GUEST:
         mOptFlags.guest |= flag->second.value;
         break;
      case OptFlagInfo::OPTFLAG_HOST:
         mOptFlags.host |= flag->second.value;
         break;
      }
   }
}

void
BinrecBackend::setVerifyEnabled(bool enabled,
                                uint32_t address)
{
   mVerifyEnabled = enabled;
   mVerifyAddress = address;
}

} // namespace jit

} // namespace cpu
