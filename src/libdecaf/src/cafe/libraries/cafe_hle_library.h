#pragma once
#include "cafe_hle_library_symbol.h"
#include "cafe_hle_library_typeinfo.h"

#include <libcpu/state.h>

#include <map>
#include <string>
#include <vector>

namespace cafe::hle
{

struct UnimplementedLibraryFunction
{
   class Library* library = nullptr;
   std::string name;
   uint32_t syscallID = 0xFFFFFFFFu;
   virt_addr value;
};

enum class LibraryId
{
   avm,
   camera,
   coreinit,
   dc,
   dmae,
   drmapp,
   erreula,
   gx2,
   h264,
   lzma920,
   mic,
   nfc,
   nio_prof,
   nlibcurl,
   nlibnss2,
   nlibnss,
   nn_acp,
   nn_ac,
   nn_act,
   nn_aoc,
   nn_boss,
   nn_ccr,
   nn_cmpt,
   nn_dlp,
   nn_ec,
   nn_fp,
   nn_hai,
   nn_hpad,
   nn_idbe,
   nn_ndm,
   nn_nets2,
   nn_nfp,
   nn_nim,
   nn_olv,
   nn_pdm,
   nn_save,
   nn_sl,
   nn_spm,
   nn_temp,
   nn_uds,
   nn_vctl,
   nsysccr,
   nsyshid,
   nsyskbd,
   nsysnet,
   nsysuhs,
   nsysuvd,
   ntag,
   padscore,
   proc_ui,
   sndcore2,
   snd_core,
   snduser2,
   snd_user,
   swkbd,
   sysapp,
   tcl,
   tve,
   uac,
   uac_rpl,
   usb_mic,
   uvc,
   uvd,
   vpadbase,
   vpad,
   zlib125,
   Max,
};

class Library
{
public:
   // We have 3 default suymbols: NULL, .text, .data
   static constexpr auto BaseSymbolIndex = uint32_t { 3 };

   static cpu::Core *
   handleUnknownSystemCall(cpu::Core *state,
                           uint32_t id);

public:
   Library(LibraryId id, std::string name) :
      mID(id), mName(std::move(name))
   {
   }

   virtual ~Library() = default;

   LibraryId
   id() const
   {
      return mID;
   }

   const std::string &
   name() const
   {
      return mName;
   }

   virt_addr
   findSymbolAddress(std::string_view name) const
   {
      auto itr = mSymbolMap.find(name);
      if (itr == mSymbolMap.end()) {
         return virt_addr { 0 };
      }

      return itr->second->address;
   }

   LibrarySymbol *
   findSymbol(std::string_view name) const
   {
      auto itr = mSymbolMap.find(name);
      if (itr == mSymbolMap.end()) {
         return nullptr;
      }

      return itr->second.get();
   }

   LibrarySymbol *
   findSymbol(virt_addr addr) const
   {
      for (auto &[name, symbol] : mSymbolMap) {
         if (symbol->address == addr) {
            return symbol.get();
         }
      }

      return nullptr;
   }

   const std::vector<uint8_t> &
   getGeneratedRpl() const
   {
      return mGeneratedRpl;
   }

   void
   generate();

   void
   relocate(virt_addr textBaseAddress,
            virt_addr dataBaseAddress);

   void
   addUnimplementedFunctionExport(UnimplementedLibraryFunction *unimpl)
   {
      mUnimplementedFunctionExports.push_back(unimpl);
   }

   UnimplementedLibraryFunction *
   findUnimplementedFunctionExport(std::string_view name)
   {
      for (auto unimpl : mUnimplementedFunctionExports) {
         if (unimpl->name == name) {
            return unimpl;
         }
      }

      return nullptr;
   }

   const auto &
   getSymbolMap() const
   {
      return mSymbolMap;
   }

   void
   registerLibraryDependency(const char *name)
   {
      mLibraryDependencies.push_back(name);
   }

   void
   registerSymbol(const std::string &name,
                  std::unique_ptr<LibrarySymbol> symbol)
   {
      decaf_check(mSymbolMap.find(name) == mSymbolMap.end());
      symbol->index = BaseSymbolIndex + static_cast<uint32_t>(mSymbolMap.size());
      symbol->name = name;
      mSymbolMap.emplace(name, std::move(symbol));
   }

   void
   registerTypeInfo(LibraryTypeInfo &&typeInfo)
   {
      mTypeInfo.emplace_back(std::move(typeInfo));
   }

   void
   setEntryPointSymbolName(const std::string& name)
   {
      mEntryPointSymbolName = name;
   }

protected:
   virtual void
   registerSymbols() = 0;

   void
   registerSystemCalls();

   void
   generateRpl();

private:
   LibraryId mID;
   std::string mName;
   std::vector<std::string> mLibraryDependencies;
   std::map<std::string, std::unique_ptr<LibrarySymbol>, std::less<>> mSymbolMap;
   std::vector<LibraryTypeInfo> mTypeInfo;
   std::vector<uint8_t> mGeneratedRpl;
   std::vector<UnimplementedLibraryFunction *> mUnimplementedFunctionExports;
   std::string mEntryPointSymbolName;
};

virt_addr
registerUnimplementedSymbol(std::string_view module,
                            std::string_view name);

void
setUnimplementedFunctionStubMemory(virt_ptr<void> base,
                                   uint32_t size);

} // namespace cafe::hle

#include "cafe_hle_library_register.h"
