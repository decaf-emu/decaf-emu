#include <cassert>
#include <vector>
#include <zlib.h>
#include "elf.h"
#include "common/bigendianview.h"
#include "common/log.h"

const unsigned elf::Header::Magic;

namespace elf
{

bool
readHeader(BigEndianView &in, Header &header)
{
   in.read(header.magic);
   in.read(header.fileClass);
   in.read(header.encoding);
   in.read(header.elfVersion);
   in.read(header.abi);
   in.read(header.pad);
   in.read(header.type);
   in.read(header.machine);
   in.read(header.version);
   in.read(header.entry);
   in.read(header.phoff);
   in.read(header.shoff);
   in.read(header.flags);
   in.read(header.ehsize);
   in.read(header.phentsize);
   in.read(header.phnum);
   in.read(header.shentsize);
   in.read(header.shnum);
   in.read(header.shstrndx);

   if (header.magic != Header::Magic) {
      gLog->error("Unexpected elf magic, found {:08x} expected {:08x}", header.magic, Header::Magic);
      return false;
   }

   if (header.fileClass != ELFCLASS32) {
      gLog->error("Unexpected elf file class, found {:02x} expected {:02x}", header.fileClass, ELFCLASS32);
      return false;
   }

   if (header.encoding != ELFDATA2MSB) {
      gLog->error("Unexpected elf encoding, found {:02x} expected {:02x}", header.encoding, ELFDATA2MSB);
      return false;
   }

   if (header.machine != EM_PPC) {
      gLog->error("Unexpected elf machine, found {:04x} expected {:04x}", header.machine, EM_PPC);
      return false;
   }

   if (header.elfVersion != EV_CURRENT) {
      gLog->error("Unexpected elf version, found {:02x} expected {:02x}", header.elfVersion, EV_CURRENT);
      return false;
   }

   assert(header.shentsize == sizeof(SectionHeader));
   return true;
}


bool
readSectionHeader(BigEndianView &in, SectionHeader &shdr)
{
   in.read(shdr.name);
   in.read(shdr.type);
   in.read(shdr.flags);
   in.read(shdr.addr);
   in.read(shdr.offset);
   in.read(shdr.size);
   in.read(shdr.link);
   in.read(shdr.info);
   in.read(shdr.addralign);
   in.read(shdr.entsize);
   return true;
}


bool
readSymbol(BigEndianView &in, Symbol &sym)
{
   in.read(sym.name);
   in.read(sym.value);
   in.read(sym.size);
   in.read(sym.info);
   in.read(sym.other);
   in.read(sym.shndx);
   return true;
}


bool
readRelocationAddend(BigEndianView &in, Rela &rela)
{
   in.read(rela.offset);
   in.read(rela.info);
   in.read(rela.addend);
   return true;
}


bool
readFileInfo(BigEndianView &in, elf::FileInfo &info)
{
   in.read(info.version);
   in.read(info.textSize);
   in.read(info.textAlign);
   in.read(info.dataSize);
   in.read(info.dataAlign);
   in.read(info.loadSize);
   in.read(info.loadAlign);
   in.read(info.tempSize);
   in.read(info.trampAdjust);
   in.read(info.sdaBase);
   in.read(info.sda2Base);
   in.read(info.stackSize);
   in.read(info.filename);
   in.read(info.flags);
   in.read(info.heapSize);
   in.read(info.tagOffset);
   in.read(info.minVersion);
   in.read(info.compressionLevel);
   in.read(info.trampAddition);
   in.read(info.fileInfoPad);
   in.read(info.cafeSdkVersion);
   in.read(info.cafeSdkRevision);
   in.read(info.tlsModuleIndex);
   in.read(info.tlsAlignShift);
   in.read(info.runtimeFileInfoSize);
   return true;
}


bool
readSectionHeaders(BigEndianView &in, Header &header, std::vector<XSection>& sections)
{
   sections.resize(header.shnum);

   for (auto i = 0u; i < sections.size(); ++i) {
      auto &sectionHeader = sections[i].header;
      in.seek(header.shoff + header.shentsize * i);
      if (!readSectionHeader(in, sectionHeader)) {
         return false;
      }
   }

   return true;
}


bool
readSectionData(BigEndianView &in, const SectionHeader& header, std::vector<uint8_t> &data)
{
   if (header.type == SHT_NOBITS || header.size == 0) {
      data.clear();
      return true;
   }

   if (header.flags & SHF_DEFLATED) {
      auto stream = z_stream{};
      auto ret = Z_OK;

      // Read the original size
      in.seek(header.offset);
      data.resize(in.read<uint32_t>());

      // Inflate
      memset(&stream, 0, sizeof(stream));
      stream.zalloc = Z_NULL;
      stream.zfree = Z_NULL;
      stream.opaque = Z_NULL;

      ret = inflateInit(&stream);

      if (ret != Z_OK) {
         gLog->error("Couldn't decompress .rpx section because inflateInit returned {}", ret);
         data.clear();
      } else {
         stream.avail_in = header.size;
         stream.next_in = const_cast<Bytef*>(in.readRaw<Bytef>(header.size));
         stream.avail_out = static_cast<uInt>(data.size());
         stream.next_out = reinterpret_cast<Bytef*>(data.data());

         ret = inflate(&stream, Z_FINISH);

         if (ret != Z_OK && ret != Z_STREAM_END) {
            gLog->error("Couldn't decompress .rpx section because inflate returned {}", ret);
            data.clear();
         }

         inflateEnd(&stream);
      }
   } else {
      data.resize(header.size);
      in.seek(header.offset);
      in.read(data.data(), header.size);
   }

   return data.size() > 0;
}

} // namespace elf
