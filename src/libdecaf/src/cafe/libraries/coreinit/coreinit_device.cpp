#include "coreinit.h"
#include "coreinit_device.h"
#include "coreinit_memory.h"

namespace cafe::coreinit
{

constexpr auto NumDevices = 16u;

struct StaticDeviceData
{
   be2_array<virt_addr, NumDevices> deviceTable;
};

static virt_ptr<StaticDeviceData>
sDeviceData = nullptr;

uint16_t
OSReadRegister16(OSDeviceID device,
                 uint32_t id)
{
   auto deviceBaseAddr = sDeviceData->deviceTable[device];
   if (!deviceBaseAddr) {
      return 0;
   }

   return *virt_cast<uint16_t *>(deviceBaseAddr + id * 4);
}

uint32_t
OSReadRegister32Ex(OSDeviceID device,
                   uint32_t id)
{
   auto deviceBaseAddr = sDeviceData->deviceTable[device];
   if (!deviceBaseAddr) {
      return 0;
   }

   return *virt_cast<uint32_t *>(deviceBaseAddr + id * 4);
}

void
OSWriteRegister16(OSDeviceID device,
                  uint32_t id,
                  uint16_t value)
{
   auto deviceBaseAddr = sDeviceData->deviceTable[device];
   if (deviceBaseAddr) {
      *virt_cast<uint16_t *>(deviceBaseAddr + id * 4) = value;
   }
}

void
OSWriteRegister32Ex(OSDeviceID device,
                    uint32_t id,
                    uint32_t value)
{
   auto deviceBaseAddr = sDeviceData->deviceTable[device];
   if (deviceBaseAddr) {
      *virt_cast<uint32_t *>(deviceBaseAddr + id * 4) = value;
   }
}

namespace internal
{

void
initialiseDeviceTable()
{
   sDeviceData->deviceTable.fill(virt_addr { 0 });
   sDeviceData->deviceTable[OSDeviceID::VI] = OSPhysicalToEffectiveUncached(phys_addr { 0x0C1E0000 });
   sDeviceData->deviceTable[OSDeviceID::DSP] = OSPhysicalToEffectiveUncached(phys_addr { 0x0C280000 });
   sDeviceData->deviceTable[OSDeviceID::GFXSP] = OSPhysicalToEffectiveUncached(phys_addr { 0x0C200000 });

   sDeviceData->deviceTable[OSDeviceID::LATTE_REGS] = OSPhysicalToEffectiveUncached(phys_addr { 0x0D000000 });
   sDeviceData->deviceTable[OSDeviceID::LEGACY_SI] = OSPhysicalToEffectiveUncached(phys_addr { 0x0D006400 });
   sDeviceData->deviceTable[OSDeviceID::LEGACY_AI_I2S3] = OSPhysicalToEffectiveUncached(phys_addr { 0x0D006C00 });
   sDeviceData->deviceTable[OSDeviceID::LEGACY_AI_I2S5] = OSPhysicalToEffectiveUncached(phys_addr { 0x0D006E00 });
   sDeviceData->deviceTable[OSDeviceID::LEGACY_EXI] = OSPhysicalToEffectiveUncached(phys_addr { 0x0D006800 });
}

} // namespace internal

void
Library::registerDeviceSymbols()
{
   RegisterFunctionExport(OSReadRegister16);
   RegisterFunctionExport(OSWriteRegister16);
   RegisterFunctionExportName("__OSReadRegister32Ex", OSReadRegister32Ex);
   RegisterFunctionExportName("__OSWriteRegister32Ex", OSWriteRegister32Ex);

   RegisterDataInternal(sDeviceData);
}

} // namespace cafe::coreinit
