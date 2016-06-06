#include "kernel_internal.h"
#include "system.h"
#include "modules/coreinit/coreinit.h"
#include "modules/coreinit/coreinit_core.h"
#include "modules/coreinit/coreinit_memheap.h"
#include "modules/coreinit/coreinit_scheduler.h"
#include "modules/coreinit/coreinit_systeminfo.h"
#include "modules/erreula/erreula.h"
#include "modules/gx2/gx2.h"
#include "modules/mic/mic.h"
#include "modules/nn_ac/nn_ac.h"
#include "modules/nn_acp/nn_acp.h"
#include "modules/nn_act/nn_act.h"
#include "modules/nn_boss/nn_boss.h"
#include "modules/nn_fp/nn_fp.h"
#include "modules/nn_ndm/nn_ndm.h"
#include "modules/nn_nfp/nn_nfp.h"
#include "modules/nn_olv/nn_olv.h"
#include "modules/nn_save/nn_save.h"
#include "modules/nn_temp/nn_temp.h"
#include "modules/nsysnet/nsysnet.h"
#include "modules/proc_ui/proc_ui.h"
#include "modules/padscore/padscore.h"
#include "modules/snd_core/snd_core.h"
#include "modules/swkbd/swkbd.h"
#include "modules/sysapp/sysapp.h"
#include "modules/vpad/vpad.h"
#include "modules/zlib125/zlib125.h"

namespace kernel
{

void initialise_hle_modules()
{
   coreinit::Module::RegisterFunctions();
   nn::erreula::Module::RegisterFunctions();
   gx2::Module::RegisterFunctions();
   mic::Module::RegisterFunctions();
   nn::ac::Module::RegisterFunctions();
   nn::acp::Module::RegisterFunctions();
   nn::act::Module::RegisterFunctions();
   nn::boss::Module::RegisterFunctions();
   nn::fp::Module::RegisterFunctions();
   nn::ndm::Module::RegisterFunctions();
   nn::nfp::Module::RegisterFunctions();
   nn::olv::Module::RegisterFunctions();
   nn::save::Module::RegisterFunctions();
   nn::temp::Module::RegisterFunctions();
   nsysnet::Module::RegisterFunctions();
   padscore::Module::RegisterFunctions();
   proc_ui::Module::RegisterFunctions();
   snd_core::Module::RegisterFunctions();
   nn::swkbd::Module::RegisterFunctions();
   sysapp::Module::RegisterFunctions();
   vpad::Module::RegisterFunctions();
   zlib125::Module::RegisterFunctions();

   gSystem.registerModule("coreinit.rpl", new coreinit::Module{});
   gSystem.registerModule("erreula.rpl", new nn::erreula::Module{});
   gSystem.registerModule("gx2.rpl", new gx2::Module{});
   gSystem.registerModule("mic.rpl", new mic::Module{});
   gSystem.registerModule("nn_ac.rpl", new nn::ac::Module{});
   gSystem.registerModule("nn_acp.rpl", new nn::acp::Module{});
   gSystem.registerModule("nn_act.rpl", new nn::act::Module{});
   gSystem.registerModule("nn_boss.rpl", new nn::boss::Module{});
   gSystem.registerModule("nn_fp.rpl", new nn::fp::Module{});
   gSystem.registerModule("nn_nfp.rpl", new nn::nfp::Module{});
   gSystem.registerModule("nn_ndm.rpl", new nn::ndm::Module{});
   gSystem.registerModule("nn_olv.rpl", new nn::olv::Module{});
   gSystem.registerModule("nn_save.rpl", new nn::save::Module{});
   gSystem.registerModule("nn_temp.rpl", new nn::temp::Module{});
   gSystem.registerModule("nsysnet.rpl", new nsysnet::Module{});
   gSystem.registerModule("padscore.rpl", new padscore::Module{});
   gSystem.registerModule("proc_ui.rpl", new proc_ui::Module{});
   gSystem.registerModule("snd_core.rpl", new snd_core::Module{});
   gSystem.registerModuleAlias("snd_core.rpl", "sndcore2.rpl");
   gSystem.registerModule("swkbd.rpl", new nn::swkbd::Module{});
   gSystem.registerModule("sysapp.rpl", new sysapp::Module{});
   gSystem.registerModule("vpad.rpl", new vpad::Module{});
   gSystem.registerModule("zlib125.rpl", new zlib125::Module{});
}

} // namespace kernel