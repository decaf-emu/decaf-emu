#include "ios_fs.h"
#include "ios_fs_fsa_thread.h"

namespace ios::fs
{

static void
fs_svc_thread()
{
   /*
   /dev/df
   /dev/atfs
   /dev/isfs
   /dev/wfs
   /dev/pcfs
   /dev/rbfs
   /dev/fat
   /dev/fla
   /dev/ums
   /dev/ahcimgr
   /dev/shdd
   /dev/md
   /dev/scfm
   /dev/mmc
   /dev/timetrace
   /dev/tcp_pcfs
   */
}

Error
processEntryPoint(phys_ptr<void> context)
{
   auto error = internal::startFsaThread();
   if (error < Error::OK) {
      return error;
   }

   return Error::OK;
}

} // namespace ios