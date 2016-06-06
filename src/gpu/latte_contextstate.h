#pragma once
#include "common/structsize.h"
#include "types.h"
#include "latte_registers.h"

#pragma pack(push, 1)

namespace latte
{

struct ContextState
{
   uint32_t config[0xB00];
   uint32_t context[0x400];
   uint32_t alu[0x800];
   uint32_t loop[0x60];
   PADDING((0x80 - 0x60) * 4);
   uint32_t resource[0xD9E];
   PADDING((0xDC0 - 0xD9E) * 4);
   uint32_t sampler[0xA2];
   PADDING((0xC0 - 0xA2) * 4);

   uint32_t getRegister(latte::Register reg)
   {
      if (reg >= latte::Register::ConfigRegisterBase && reg < latte::Register::ConfigRegisterEnd) {
         return config[(reg - latte::Register::ConfigRegisterBase) / 4];
      } else if (reg >= latte::Register::ContextRegisterBase && reg < latte::Register::ContextRegisterEnd) {
         return context[(reg - latte::Register::ContextRegisterBase) / 4];
      } else if (reg >= latte::Register::AluConstRegisterBase && reg < latte::Register::AluConstRegisterEnd) {
         return alu[(reg - latte::Register::AluConstRegisterBase) / 4];
      } else if (reg >= latte::Register::LoopConstRegisterBase && reg < latte::Register::LoopConstRegisterEnd) {
         return loop[(reg - latte::Register::LoopConstRegisterBase) / 4];
      } else if (reg >= latte::Register::ResourceRegisterBase && reg < latte::Register::ResourceRegisterEnd) {
         return resource[(reg - latte::Register::ResourceRegisterBase) / 4];
      } else if (reg >= latte::Register::SamplerRegisterBase && reg < latte::Register::SamplerRegisterEnd) {
         return sampler[(reg - latte::Register::SamplerRegisterBase) / 4];
      }

      return 0;
   }

   void setRegister(latte::Register reg, uint32_t value)
   {
      if (reg >= latte::Register::ConfigRegisterBase && reg < latte::Register::ConfigRegisterEnd) {
         config[(reg - latte::Register::ConfigRegisterBase) / 4] = value;
      } else if (reg >= latte::Register::ContextRegisterBase && reg < latte::Register::ContextRegisterEnd) {
         context[(reg - latte::Register::ContextRegisterBase) / 4] = value;
      } else if (reg >= latte::Register::AluConstRegisterBase && reg < latte::Register::AluConstRegisterEnd) {
         alu[(reg - latte::Register::AluConstRegisterBase) / 4] = value;
      } else if (reg >= latte::Register::LoopConstRegisterBase && reg < latte::Register::LoopConstRegisterEnd) {
         loop[(reg - latte::Register::LoopConstRegisterBase) / 4] = value;
      } else if (reg >= latte::Register::ResourceRegisterBase && reg < latte::Register::ResourceRegisterEnd) {
         resource[(reg - latte::Register::ResourceRegisterBase) / 4] = value;
      } else if (reg >= latte::Register::SamplerRegisterBase && reg < latte::Register::SamplerRegisterEnd) {
         sampler[(reg - latte::Register::SamplerRegisterBase) / 4] = value;
      }
   }
};
CHECK_OFFSET(ContextState, 0, config);
CHECK_OFFSET(ContextState, 0x2C00, context);
CHECK_OFFSET(ContextState, 0x3C00, alu);
CHECK_OFFSET(ContextState, 0x5C00, loop);
CHECK_OFFSET(ContextState, 0x5E00, resource);
CHECK_OFFSET(ContextState, 0x9500, sampler);
CHECK_SIZE(ContextState, 0x9800);

} // namespace latte

#pragma pack(pop)
