#pragma once
#include "cafe_ppc_interface.h"

#include <libcpu/state.h>
#include <libcpu/functionpointer.h>
#include <libcpu/be2_val.h>

namespace cafe::detail
{

template<auto regIndex>
inline uint32_t
readGpr(cpu::Core* core)
{
   if constexpr (regIndex <= 10) {
      return core->gpr[regIndex];
   } else {
      // Args come after the backchain from the caller (8 bytes).
      auto addr = core->gpr[1] + 8 + 4 * static_cast<uint32_t>(regIndex - 11);
      return *virt_cast<uint32_t*>(virt_addr { addr });
   }
}

template<typename ArgType, RegisterType regType, auto regIndex>
inline ArgType
readParam(cpu::Core* core,
          param_info_t<ArgType, regType, regIndex>)
{
   using ValueType = std::remove_cv_t<ArgType>;
   if constexpr (regType == RegisterType::Gpr32) {
      auto value = readGpr<regIndex>(core);

      if constexpr (is_virt_ptr<ValueType>::value) {
         return virt_cast<typename ArgType::value_type*>(static_cast<virt_addr>(value));
      } else if constexpr (is_phys_ptr<ValueType>::value) {
         return phys_cast<typename ArgType::value_type*>(static_cast<phys_addr>(value));
      } else if constexpr (is_virt_func_ptr<ValueType>::value) {
         return virt_func_cast<typename ArgType::function_type>(static_cast<virt_addr>(value));
      } else if constexpr (is_bitfield_type<ValueType>::value) {
         return ArgType::get(value);
      } else if constexpr (std::is_same<bool, ValueType>::value) {
         return !!value;
      } else {
         return static_cast<ArgType>(value);
      }
   } else if constexpr (regType == RegisterType::Gpr64) {
      auto hi = static_cast<uint64_t>(readGpr<regIndex>(core)) << 32;
      auto lo = static_cast<uint64_t>(readGpr<regIndex + 1>(core));
      return static_cast<ArgType>(hi | lo);
   } else if constexpr (regType == RegisterType::Fpr) {
      return static_cast<ArgType>(core->fpr[regIndex].paired0);
   } else if constexpr (regType == RegisterType::VarArgs) {
      return var_args { ((regIndex - 3) & 0xFF), (regIndex >> 8) };
   }
}

template<typename ArgType, RegisterType regType, auto regIndex>
inline void
writeParam(cpu::Core *core,
           param_info_t<ArgType, regType, regIndex>,
           const std::remove_cv_t<ArgType> &value)
{
   using ValueType = std::remove_cv_t<ArgType>;
   static_assert(regType != RegisterType::VarArgs,
                 "writeParam not supported with VarArgs");

   if constexpr (regType == RegisterType::Gpr32) {
      if constexpr (is_virt_ptr<ValueType>::value) {
         core->gpr[regIndex] = static_cast<uint32_t>(virt_cast<virt_addr>(value));
      } else if constexpr (is_phys_ptr<ValueType>::value) {
         core->gpr[regIndex] = static_cast<uint32_t>(phys_cast<phys_addr>(value));
      } else if constexpr (is_virt_func_ptr<ValueType>::value) {
         core->gpr[regIndex] = static_cast<uint32_t>(virt_func_cast<virt_addr>(value));
      } else if constexpr (is_bitfield_type<ValueType>::value) {
         core->gpr[regIndex] = static_cast<uint32_t>(value.value);
      } else if constexpr (std::is_same<bool, ValueType>::value) {
         core->gpr[regIndex] = static_cast<uint32_t>(value ? 1 : 0);
      } else {
         core->gpr[regIndex] = static_cast<uint32_t>(value);
      }
   } else if constexpr (regType == RegisterType::Gpr64) {
      core->gpr[regIndex] = static_cast<uint32_t>((value >> 32) & 0xFFFFFFFF);
      core->gpr[regIndex + 1] = static_cast<uint32_t>(value & 0xFFFFFFFF);
   } else if constexpr (regType == RegisterType::Fpr) {
      core->fpr[regIndex].paired0 = static_cast<double>(value);
   }
}

template<typename ArgType, RegisterType regType, auto regIndex>
inline void
writeParam(cpu::Core *core,
           param_info_t<ArgType, regType, regIndex> p,
           const be2_val<ArgType> &value)
{
   writeParam<ArgType, regType, regIndex>(core, p, value.value());
}

} // namespace cafe
