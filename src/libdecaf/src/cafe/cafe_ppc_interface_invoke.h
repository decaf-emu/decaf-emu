#pragma once
#include "cafe_ppc_interface.h"

#include <libcpu/cpu.h>
#include <libcpu/functionpointer.h>
#include <libcpu/be2_val.h>

namespace cafe
{

namespace detail
{

template<typename ArgType, RegisterType regType, auto regIndex>
inline ArgType
readParam(cpu::Core *core,
          param_info_t<ArgType, regType, regIndex>)
{
   using ValueType = std::remove_cv_t<ArgType>;
   if constexpr (regType == RegisterType::Gpr32) {
      if constexpr (is_virt_ptr<ValueType>::value) {
         return virt_cast<typename ArgType::value_type *>(static_cast<virt_addr>(core->gpr[regIndex]));
      } else if constexpr (is_phys_ptr<ValueType>::value) {
         return phys_cast<typename ArgType::value_type *>(static_cast<phys_addr>(core->gpr[regIndex]));
      } else if constexpr (is_virt_func_ptr<ValueType>::value) {
         return virt_func_cast<typename ArgType::function_type>(static_cast<virt_addr>(core->gpr[regIndex]));
      } else if constexpr (is_bitfield_type<ValueType>::value) {
         return ArgType::get(core->gpr[regIndex]);
      } else {
         return static_cast<ArgType>(core->gpr[regIndex]);
      }
   } else if constexpr (regType == RegisterType::Gpr64) {
      return static_cast<ArgType>((static_cast<uint64_t>(core->gpr[regIndex]) << 32) | static_cast<uint64_t>(core->gpr[regIndex + 1]));
   } else if constexpr (regType == RegisterType::Fpr) {
      return static_cast<ArgType>(core->fpr[regIndex].paired0);
   } else if constexpr (regType == RegisterType::VarArgs) {
      return var_args { (regIndex & 0xFF), (regIndex >> 8) };
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

template<typename HostFunctionType, typename FunctionTraitsType, std::size_t... I>
constexpr void
invoke_host_impl(cpu::Core *core,
                 HostFunctionType &&func,
                 FunctionTraitsType &&,
                 std::index_sequence<I...>)
{
   auto param_info = typename FunctionTraitsType::param_info { };

   if constexpr (FunctionTraitsType::has_return_value) {
      auto return_info = typename FunctionTraitsType::return_info { };

      if constexpr (FunctionTraitsType::is_member_function) {
         auto obj = readParam(core, typename FunctionTraitsType::object_info { });
         writeParam(core,
                    return_info,
                    (obj.getRawPointer()->*func)(readParam(core, std::get<I>(param_info))...));
      } else {
         writeParam(core,
                    return_info,
                    func(readParam(core, std::get<I>(param_info))...));
      }
   } else {
      if constexpr (FunctionTraitsType::is_member_function) {
         auto obj = readParam(core, typename FunctionTraitsType::object_info { });
         (obj.getRawPointer()->*func)(readParam(core, std::get<I>(param_info))...);
      } else {
         func(readParam(core, std::get<I>(param_info))...);
      }
   }
}

template<typename FunctionTraitsType, std::size_t... I, typename... ArgTypes>
constexpr decltype(auto)
invoke_guest_impl(cpu::Core *core,
                  cpu::VirtualAddress address,
                  FunctionTraitsType &&,
                  std::index_sequence<I...>,
                  const ArgTypes &... args)
{
   if constexpr (FunctionTraitsType::num_args > 0) {
      // Write arguments to registers
      auto param_info = typename FunctionTraitsType::param_info { };
      (writeParam(core, std::get<I>(param_info), args), ...);
   }

   // Save a stack pointer so we can compare after to ensure no stack overflow
   auto origStackAddress = core->gpr[1];

   // Write LR to the previous backchain address
   auto backchainAddress = *virt_cast<virt_addr *>(virt_addr { origStackAddress });

   // This might be the very first call on a core...
   if (backchainAddress) {
      auto backchainPtr = virt_cast<virt_addr *>(virt_addr { backchainAddress + 4 });
      *backchainPtr = core->lr;
   }

   // Save state
   auto cia = core->cia;
   auto nia = core->nia;

   // Set state to our function to call
   core->cia = 0;
   core->nia = address.getAddress();

   // Start executing!
   cpu::this_core::executeSub();

   // Grab the most recent core state as it may have changed.
   core = cpu::this_core::state();

   // Restore state
   core->cia = cia;
   core->nia = nia;

   // Lets verify we did not corrupt the SP
   decaf_check(core->gpr[1] == origStackAddress);

   // Return the result
   if constexpr (FunctionTraitsType::has_return_value) {
      return readParam(core, typename FunctionTraitsType::return_info { });
   }
}

} // namespace detail

// Invoke a host function from a guest context
template<typename FunctionType>
void
invoke(cpu::Core *core,
       FunctionType fn)
{
   using func_traits = detail::function_traits<FunctionType>;
   invoke_host_impl(core,
                    fn,
                    func_traits { },
                    std::make_index_sequence<func_traits::num_args> {});
}

// Invoke a guest function from a host context
template<typename FunctionType, typename... ArgTypes>
decltype(auto)
invoke(cpu::Core *core,
       cpu::FunctionPointer<cpu::VirtualAddress, FunctionType> fn,
       const ArgTypes &... args)
{
   using func_traits = detail::function_traits<FunctionType>;
   return invoke_guest_impl(core,
                            fn.getAddress(),
                            func_traits { },
                            std::make_index_sequence<func_traits::num_args> {},
                            args...);
}

// Invoke a be2_val guest function from a host context
template<typename FunctionType, typename... ArgTypes>
decltype(auto)
invoke(cpu::Core *core,
       be2_val<cpu::FunctionPointer<cpu::VirtualAddress, FunctionType>> fn,
       const ArgTypes &... args)
{
   using func_traits = detail::function_traits<FunctionType>;
   return invoke_guest_impl(core,
                            fn.getAddress(),
                            func_traits { },
                            std::make_index_sequence<func_traits::num_args> {},
                            args...);
}

} // namespace cafe
