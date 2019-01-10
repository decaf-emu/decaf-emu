#include "cafe_hle.h"

#include "avm/avm.h"
#include "camera/camera.h"
#include "coreinit/coreinit.h"
#include "dc/dc.h"
#include "dmae/dmae.h"
#include "drmapp/drmapp.h"
#include "erreula/erreula.h"
#include "gx2/gx2.h"
#include "h264/h264.h"
#include "lzma920/lzma920.h"
#include "mic/mic.h"
#include "nfc/nfc.h"
#include "nio_prof/nio_prof.h"
#include "nlibcurl/nlibcurl.h"
#include "nlibnss2/nlibnss2.h"
#include "nlibnss/nlibnss.h"
#include "nn_acp/nn_acp.h"
#include "nn_ac/nn_ac.h"
#include "nn_act/nn_act.h"
#include "nn_aoc/nn_aoc.h"
#include "nn_boss/nn_boss.h"
#include "nn_ccr/nn_ccr.h"
#include "nn_cmpt/nn_cmpt.h"
#include "nn_dlp/nn_dlp.h"
#include "nn_ec/nn_ec.h"
#include "nn_fp/nn_fp.h"
#include "nn_hai/nn_hai.h"
#include "nn_hpad/nn_hpad.h"
#include "nn_idbe/nn_idbe.h"
#include "nn_ndm/nn_ndm.h"
#include "nn_nets2/nn_nets2.h"
#include "nn_nfp/nn_nfp.h"
#include "nn_nim/nn_nim.h"
#include "nn_olv/nn_olv.h"
#include "nn_pdm/nn_pdm.h"
#include "nn_save/nn_save.h"
#include "nn_sl/nn_sl.h"
#include "nn_spm/nn_spm.h"
#include "nn_temp/nn_temp.h"
#include "nn_uds/nn_uds.h"
#include "nn_vctl/nn_vctl.h"
#include "nsysccr/nsysccr.h"
#include "nsyshid/nsyshid.h"
#include "nsyskbd/nsyskbd.h"
#include "nsysnet/nsysnet.h"
#include "nsysuhs/nsysuhs.h"
#include "nsysuvd/nsysuvd.h"
#include "ntag/ntag.h"
#include "padscore/padscore.h"
#include "proc_ui/proc_ui.h"
#include "sndcore2/sndcore2.h"
#include "snd_core/snd_core.h"
#include "snduser2/snduser2.h"
#include "snd_user/snd_user.h"
#include "swkbd/swkbd.h"
#include "sysapp/sysapp.h"
#include "tcl/tcl.h"
#include "tve/tve.h"
#include "uac/uac.h"
#include "uac_rpl/uac_rpl.h"
#include "usb_mic/usb_mic.h"
#include "uvc/uvc.h"
#include "uvd/uvd.h"
#include "vpadbase/vpadbase.h"
#include "vpad/vpad.h"
#include "zlib125/zlib125.h"

#include "decaf_config.h"
#include "decaf_configstorage.h"

#include <common/log.h>
#include <array>
#include <regex>

namespace cafe::hle
{

volatile bool FunctionTraceEnabled = false;

static std::array<Library *, static_cast<size_t>(LibraryId::Max)>
sLibraries;

static void
registerLibrary(Library *library)
{
   sLibraries[static_cast<size_t>(library->id())] = library;
   library->generate();
}

void
initialiseLibraries()
{
   sLibraries.fill(nullptr);
   registerLibrary(new avm::Library { });
   registerLibrary(new camera::Library { });
   registerLibrary(new coreinit::Library { });
   registerLibrary(new dc::Library { });
   registerLibrary(new dmae::Library { });
   registerLibrary(new drmapp::Library { });
   registerLibrary(new nn_erreula::Library { });
   registerLibrary(new gx2::Library { });
   registerLibrary(new h264::Library { });
   registerLibrary(new lzma920::Library { });
   registerLibrary(new mic::Library { });
   registerLibrary(new nfc::Library { });
   registerLibrary(new nio_prof::Library { });
   registerLibrary(new nlibcurl::Library { });
   registerLibrary(new nlibnss2::Library { });
   registerLibrary(new nlibnss::Library { });
   registerLibrary(new nn_acp::Library { });
   registerLibrary(new nn_ac::Library { });
   registerLibrary(new nn_act::Library { });
   registerLibrary(new nn_aoc::Library { });
   registerLibrary(new nn_boss::Library { });
   registerLibrary(new nn_ccr::Library { });
   registerLibrary(new nn_cmpt::Library { });
   registerLibrary(new nn_dlp::Library { });
   registerLibrary(new nn_ec::Library { });
   registerLibrary(new nn_fp::Library { });
   registerLibrary(new nn_hai::Library { });
   registerLibrary(new nn_hpad::Library { });
   registerLibrary(new nn_idbe::Library { });
   registerLibrary(new nn_ndm::Library { });
   registerLibrary(new nn_nets2::Library { });
   registerLibrary(new nn_nfp::Library { });
   registerLibrary(new nn_nim::Library { });
   registerLibrary(new nn_olv::Library { });
   registerLibrary(new nn_pdm::Library { });
   registerLibrary(new nn_save::Library { });
   registerLibrary(new nn_sl::Library { });
   registerLibrary(new nn_spm::Library { });
   registerLibrary(new nn_temp::Library { });
   registerLibrary(new nn_uds::Library { });
   registerLibrary(new nn_vctl::Library { });
   registerLibrary(new nsysccr::Library { });
   registerLibrary(new nsyshid::Library { });
   registerLibrary(new nsyskbd::Library { });
   registerLibrary(new nsysnet::Library { });
   registerLibrary(new nsysuhs::Library { });
   registerLibrary(new nsysuvd::Library { });
   registerLibrary(new ntag::Library { });
   registerLibrary(new padscore::Library { });
   registerLibrary(new proc_ui::Library { });
   registerLibrary(new sndcore2::Library { });
   registerLibrary(new snd_core::Library { });
   registerLibrary(new snduser2::Library { });
   registerLibrary(new snd_user::Library { });
   registerLibrary(new swkbd::Library { });
   registerLibrary(new sysapp::Library { });
   registerLibrary(new tcl::Library { });
   registerLibrary(new tve::Library { });
   registerLibrary(new uac::Library { });
   registerLibrary(new uac_rpl::Library { });
   registerLibrary(new usb_mic::Library { });
   registerLibrary(new uvc::Library { });
   registerLibrary(new uvd::Library { });
   registerLibrary(new vpadbase::Library { });
   registerLibrary(new vpad::Library { });
   registerLibrary(new zlib125::Library { });

   // Register config change handler
   static std::once_flag sRegisteredConfigChangeListener;
   std::call_once(sRegisteredConfigChangeListener,
      []() {
         decaf::registerConfigChangeListener(
            [](const decaf::Settings &settings) {
               setTraceEnabled(settings.log.hle_trace);
               applyTraceFilters(settings.log.hle_trace_filters);
            });
      });

   // Apply trace config
   setTraceEnabled(decaf::config()->log.hle_trace);
   applyTraceFilters(decaf::config()->log.hle_trace_filters);
}

Library *
getLibrary(LibraryId id)
{
   return sLibraries[static_cast<size_t>(id)];
}

Library *
getLibrary(std::string_view name)
{
   for (auto library : sLibraries) {
      if (library && library->name() == name) {
         return library;
      }
   }

   return nullptr;
}

void
relocateLibrary(std::string_view name,
                virt_addr textBaseAddress,
                virt_addr dataBaseAddress)
{
   auto libraryName = std::string { name } + ".rpl";
   auto library = getLibrary(libraryName);
   if (library) {
      library->relocate(textBaseAddress, dataBaseAddress);
   }
}

void
applyTraceFilters(const std::vector<std::string> &filterStrings)
{
   struct TraceFilter2
   {
      bool enable;
      std::regex re;
   };

   std::vector<TraceFilter2> filters;
   filters.reserve(filterStrings.size());

   for (auto &filterString : filterStrings) {
      auto &filter = filters.emplace_back();

      // First character is + enable or - disable
      if (filterString[0] == '+') {
         filter.enable = true;
      } else if (filterString[0] == '-') {
         filter.enable = false;
      } else {
         gLog->error(fmt::format(
            "Invalid trace filter {}, expected to begin with + or -",
            filterString));
      }

      try {
         filter.re = filterString.c_str() + 1;
      } catch (std::regex_error e) {
         gLog->error(fmt::format(
            "Invalid trace filter {}, regex error {}",
             filterString, e.what()));
      }
   }

   auto buffer = std::string { };
   buffer.reserve(512);

   for (auto library : sLibraries) {
      auto libraryNameSize = library->name().size();
      buffer.insert(0, library->name());

      const auto &symbolMap = library->getSymbolMap();
      for (auto &[symbolName, symbol] : symbolMap) {
         if (symbol->type != LibrarySymbol::Function) {
            continue;
         }

         auto funcSymbol = static_cast<LibraryFunction *>(symbol.get());

         // Match regex against libraryName::functionName
         buffer.resize(libraryNameSize);
         buffer += "::";
         buffer += funcSymbol->name;

         for (auto &filter : filters) {
            if (std::regex_match(buffer, filter.re)) {
               funcSymbol->traceEnabled = filter.enable;
            }
         }
      }
   }
}

void
setTraceEnabled(bool enabled)
{
   FunctionTraceEnabled = enabled;
}

} // namespace cafe::hle
