#pragma once
#include <cstdint>
#include <utility>

namespace nn::ipc
{

using CommandId = int32_t;
using ServiceId = int32_t;

template<typename Service, int Id>
struct Command
{
   template<typename... ParameterTypes>
   struct Parameters
   {
      static constexpr ServiceId service = Service::id;
      static constexpr CommandId command = Id;

      using parameters = std::tuple<ParameterTypes...>;
      using response = std::tuple<>;

      // ::Response is optional
      template<typename... ResponseTypes>
      struct Response
      {
         static constexpr ServiceId service = Service::id;
         static constexpr CommandId command = Id;

         using parameters = std::tuple<ParameterTypes...>;
         using response = std::tuple<ResponseTypes...>;
      };
   };
};

} // namespace nn::ipc
