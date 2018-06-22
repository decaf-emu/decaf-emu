#include "decaf.h"
#include <QEventLoop>

#include <libdecaf/decaf.h>
#include <libdecaf/src/kernel/kernel_memory.h>
#include <libdecaf/src/modules/gx2/gx2_internal_cbpool.h>
#include <libdecaf/src/modules/gx2/gx2_state.h>
#include <libcpu/cpu.h>
#include <libcpu/be2_struct.h>
#include <libgpu/gpu_config.h>

void Decaf::start()
{
   // Initialise libdecaf logger
   decaf::config::log::to_file = true;
   decaf::config::log::to_stdout = true;
   decaf::config::log::level = "debug";
   decaf::initialiseLogging("pm4-replay-qt");

   gpu::config::debug = true;

   // We need to run the trace on a core.
   cpu::initialise();
   cpu::setCoreEntrypointHandler(
      [runner = this](cpu::Core *core) {
         if (core->id == 1) {
            runner->mainCoreEntry();
         }
      });

   cpu::start();
}

void Decaf::mainCoreEntry()
{
   mThread = QThread::currentThread();
   mGraphicsDriver = reinterpret_cast<gpu::OpenGLDriver *>(gpu::createGLDriver());

   // Setup decaf
   kernel::initialiseVirtualMemory();
   kernel::initialiseAppMemory(0x10000, 0, 0);
   auto systemHeapBounds = kernel::getVirtualRange(kernel::VirtualRegion::CafeOS);
   mHeap = new TeenyHeap { virt_cast<void *>(static_cast<virt_addr>(systemHeapBounds.start)).getRawPointer(), systemHeapBounds.size };

   // Setup pm4 command buffer pool
   auto cbPoolSize = 0x2000;
   auto cbPoolBase = mHeap->alloc(cbPoolSize, 0x100);
   gx2::internal::setMainCore();
   gx2::internal::initCommandBufferPool(reinterpret_cast<uint32_t *>(cbPoolBase), cbPoolSize / 4);

   // Inform listeners we are ready to go!
   emit started();

   // Run a Qt event loop which we can schedule tasks on
   QEventLoop loop;
   loop.exec();

   // Exit!
   mThread = nullptr;
}
