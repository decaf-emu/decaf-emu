#include "ios_device_debug.h"

#include <common/log.h>
#include <libcpu/mem.h>

namespace ios
{

IOSError
DebugDevice::open(IOSOpenMode mode)
{
   gLog->warn("{}: open({})", mName, mode);
   return IOSError::OK;
}

IOSError
DebugDevice::close()
{
   gLog->warn("{}: close()", mName);
   return IOSError::OK;
}

IOSError
DebugDevice::read(void *buffer,
                  size_t length)
{
   gLog->warn("{}: read({:08X}, {})", mName,
              mem::untranslate(buffer), length);
   return IOSError::FailInternal;
}

IOSError
DebugDevice::write(void *buffer,
                   size_t length)
{
   gLog->warn("{}: write({:08X}, {})", mName,
              mem::untranslate(buffer), length);

   return IOSError::FailInternal;
}

static void
writeMemory(fmt::MemoryWriter &out,
            const std::string &name,
            void *ptr,
            size_t size)
{
   out.write("{} [ptr = {:08X} len = {}]:", name, mem::untranslate(ptr), size);

   for (auto i = 0u; i < size; ++i) {
      if ((i % 64) == 0) {
         out.write("\n");
      }

      out.write("{:02X} ", reinterpret_cast<uint8_t *>(ptr)[i]);
   }
}

IOSError
DebugDevice::ioctl(uint32_t request,
                   void *inBuf,
                   size_t inLen,
                   void *outBuf,
                   size_t outLen)
{
   fmt::MemoryWriter w;
   w.write("{}: ioctl({}, {:08X}, {}, {:08X}, {})\n", mName,
              request,
              mem::untranslate(inBuf), inLen,
              mem::untranslate(outBuf), outLen);

   writeMemory(w, "in", inBuf, inLen);
   gLog->warn(w.str());
   return IOSError::OK;
}

IOSError
DebugDevice::ioctlv(uint32_t request,
                    size_t vecIn,
                    size_t vecOut,
                    IOSVec *vec)
{
   fmt::MemoryWriter w;
   w.write("{}: ioctlv({}, {}, {}, {:08X}): ", mName,
           request, vecIn, vecOut, mem::untranslate(vec));

   for (auto i = 0u; i < vecIn; ++i) {
      auto ptr = cpu::PhysicalPointer<void> { vec[i].paddr };
      writeMemory(w, fmt::format("\nin[{}]", i), ptr.getRawPointer(), vec[i].len);
   }

   for (auto i = vecIn; i < vecIn + vecOut; ++i) {
      auto ptr = cpu::PhysicalPointer<void> { vec[i].paddr };
      w.write("\nout [ptr = {:08X} len = {}]:", vec[i].paddr.getAddress(), vec[i].len);
      std::memset(ptr.getRawPointer(), 0, vec[i].len);
   }

   gLog->warn(w.str());
   return IOSError::OK;
}

} // namespace ios
