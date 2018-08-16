#include "zlib125.h"
#include "cafe/cafe_ppc_interface_invoke.h"
#include "cafe/libraries/coreinit/coreinit_memdefaultheap.h"

#include <common/decaf_assert.h>
#include <libcpu/mmu.h>
#include <zlib.h>

namespace cafe::zlib125
{

static std::map<uint32_t, z_stream>
gStreamMap;

// WiiU games will be using a 32bit zlib where stuff is in big endian order in memory
// this means all structures like z_streamp have to be swapped endian to a temp structure

using zlib125_alloc_func = virt_func_ptr<
   virt_ptr<void>(virt_ptr<void>, uint32_t, uint32_t)>;

using zlib125_free_func = virt_func_ptr<
   void(virt_ptr<void>, virt_ptr<void>)>;

struct zlib125_stream
{
   be2_virt_ptr<Bytef> next_in;
   be2_val<uint32_t> avail_in;
   be2_val<uint32_t> total_in;

   be2_virt_ptr<Bytef> next_out;
   be2_val<uint32_t> avail_out;
   be2_val<uint32_t> total_out;

   be2_virt_ptr<char> msg;
   be2_virt_ptr<struct internal_state> state;

   be2_val<zlib125_alloc_func> zalloc;
   be2_val<zlib125_free_func> zfree;
   be2_virt_ptr<void> opaque;

   be2_val<int32_t> data_type;
   be2_val<uint32_t> adler;
   be2_val<uint32_t> reserved;
};
CHECK_OFFSET(zlib125_stream, 0x00, next_in);
CHECK_OFFSET(zlib125_stream, 0x04, avail_in);
CHECK_OFFSET(zlib125_stream, 0x08, total_in);
CHECK_OFFSET(zlib125_stream, 0x0C, next_out);
CHECK_OFFSET(zlib125_stream, 0x10, avail_out);
CHECK_OFFSET(zlib125_stream, 0x14, total_out);
CHECK_OFFSET(zlib125_stream, 0x18, msg);
CHECK_OFFSET(zlib125_stream, 0x1C, state);
CHECK_OFFSET(zlib125_stream, 0x20, zalloc);
CHECK_OFFSET(zlib125_stream, 0x24, zfree);
CHECK_OFFSET(zlib125_stream, 0x28, opaque);
CHECK_OFFSET(zlib125_stream, 0x2C, data_type);
CHECK_OFFSET(zlib125_stream, 0x30, adler);
CHECK_OFFSET(zlib125_stream, 0x34, reserved);
CHECK_SIZE(zlib125_stream, 0x38);

struct zlib125_header
{
   be2_val<int32_t> text;
   be2_val<uint32_t> time;
   be2_val<int32_t> xflags;
   be2_val<int32_t> os;
   be2_virt_ptr<uint8_t> extra;
   be2_val<uint32_t> extra_len;
   be2_val<uint32_t> extra_max;
   be2_virt_ptr<uint8_t> name;
   be2_val<uint32_t> name_max;
   be2_virt_ptr<uint8_t> comment;
   be2_val<uint32_t> comm_max;
   be2_val<int32_t> hcrc;
   be2_val<int32_t> done;
};
CHECK_OFFSET(zlib125_header, 0x00, text);
CHECK_OFFSET(zlib125_header, 0x04, time);
CHECK_OFFSET(zlib125_header, 0x08, xflags);
CHECK_OFFSET(zlib125_header, 0x0C, os);
CHECK_OFFSET(zlib125_header, 0x10, extra);
CHECK_OFFSET(zlib125_header, 0x14, extra_len);
CHECK_OFFSET(zlib125_header, 0x18, extra_max);
CHECK_OFFSET(zlib125_header, 0x1C, name);
CHECK_OFFSET(zlib125_header, 0x20, name_max);
CHECK_OFFSET(zlib125_header, 0x24, comment);
CHECK_OFFSET(zlib125_header, 0x28, comm_max);
CHECK_OFFSET(zlib125_header, 0x2C, hcrc);
CHECK_OFFSET(zlib125_header, 0x30, done);
CHECK_SIZE(zlib125_header, 0x34);

static void *
zlibAllocWrapper(void *opaque,
                 unsigned items,
                 unsigned size)
{
   auto wstrm = reinterpret_cast<zlib125_stream *>(opaque);

   if (wstrm->zalloc) {
      auto ptr = cafe::invoke(cpu::this_core::state(),
                              wstrm->zalloc,
                              wstrm->opaque,
                              static_cast<uint32_t>(items),
                              static_cast<uint32_t>(size));
      return ptr.getRawPointer();
   } else {
      auto ptr = coreinit::MEMAllocFromDefaultHeap(items * size);
      return ptr.getRawPointer();
   }
}

static void
zlibFreeWrapper(void *opaque,
                void *address)
{
   auto wstrm = reinterpret_cast<zlib125_stream *>(opaque);
   auto ptr = virt_cast<void *>(cpu::translate(address));

   if (wstrm->zfree) {
      cafe::invoke(cpu::this_core::state(),
                   wstrm->zfree,
                   wstrm->opaque,
                   ptr);
   } else {
      coreinit::MEMFreeToDefaultHeap(ptr);
   }
}

z_stream *
getZStream(virt_ptr<zlib125_stream> in)
{
   auto zstream = &gStreamMap[virt_cast<virt_addr>(in).getAddress()];
   zstream->opaque = in.getRawPointer();
   zstream->zalloc = &zlibAllocWrapper;
   zstream->zfree = &zlibFreeWrapper;
   return zstream;
}

void
eraseZStream(virt_ptr<zlib125_stream> in)
{
   gStreamMap.erase(virt_cast<virt_addr>(in).getAddress());
}

static int
zlib125_deflate(virt_ptr<zlib125_stream> wstrm,
                int32_t flush)
{
   auto zstrm = getZStream(wstrm);
   zstrm->next_in = wstrm->next_in.getRawPointer();
   zstrm->avail_in = wstrm->avail_in;
   zstrm->total_in = wstrm->total_in;

   zstrm->next_out = wstrm->next_out.getRawPointer();
   zstrm->avail_out = wstrm->avail_out;
   zstrm->total_out = wstrm->total_out;

   zstrm->data_type = wstrm->data_type;
   zstrm->adler = wstrm->adler;

   auto result = deflate(zstrm, flush);

   wstrm->next_in = virt_cast<Bytef *>(cpu::translate(zstrm->next_in));
   wstrm->avail_in = zstrm->avail_in;
   wstrm->total_in = zstrm->total_in;

   wstrm->next_out = virt_cast<Bytef *>(cpu::translate(zstrm->next_out));
   wstrm->avail_out = zstrm->avail_out;
   wstrm->total_out = zstrm->total_out;

   wstrm->data_type = zstrm->data_type;
   wstrm->adler = zstrm->adler;

   return result;
}

static int
zlib125_deflateInit_(virt_ptr<zlib125_stream> wstrm,
                     int32_t level,
                     virt_ptr<const char> version,
                     int32_t stream_size)
{
   decaf_check(sizeof(zlib125_stream) == stream_size);

   auto zstrm = getZStream(wstrm);
   auto result = deflateInit_(zstrm, level,
                              version.getRawPointer(), sizeof(z_stream));

   wstrm->msg = nullptr;
   return result;
}

static int
zlib125_deflateInit2_(virt_ptr<zlib125_stream> wstrm,
                      int32_t level,
                      int32_t method,
                      int32_t windowBits,
                      int32_t memLevel,
                      int32_t strategy,
                      virt_ptr<const char> version,
                      int32_t stream_size)
{
   decaf_check(sizeof(zlib125_stream) == stream_size);

   auto zstrm = getZStream(wstrm);
   auto result = deflateInit2_(zstrm, level, method, windowBits, memLevel,
                               strategy, version.getRawPointer(),
                               sizeof(z_stream));

   wstrm->msg = nullptr;
   return result;
}

static uint32_t
zlib125_deflateBound(virt_ptr<zlib125_stream> wstrm,
                     uint32_t sourceLen)
{
   auto zstrm = getZStream(wstrm);
   return deflateBound(zstrm, sourceLen);
}

static int
zlib125_deflateReset(virt_ptr<zlib125_stream> wstrm)
{
   auto zstrm = getZStream(wstrm);
   return deflateReset(zstrm);
}

static int
zlib125_deflateEnd(virt_ptr<zlib125_stream> wstrm)
{
   auto zstrm = getZStream(wstrm);
   return deflateEnd(zstrm);
}

static int
zlib125_inflate(virt_ptr<zlib125_stream> wstrm,
                int32_t flush)
{
   auto zstrm = getZStream(wstrm);
   zstrm->next_in = wstrm->next_in.getRawPointer();
   zstrm->avail_in = wstrm->avail_in;
   zstrm->total_in = wstrm->total_in;

   zstrm->next_out = wstrm->next_out.getRawPointer();
   zstrm->avail_out = wstrm->avail_out;
   zstrm->total_out = wstrm->total_out;

   zstrm->data_type = wstrm->data_type;
   zstrm->adler = wstrm->adler;

   auto result = inflate(zstrm, flush);

   wstrm->next_in = virt_cast<Bytef *>(cpu::translate(zstrm->next_in));
   wstrm->avail_in = zstrm->avail_in;
   wstrm->total_in = zstrm->total_in;

   wstrm->next_out = virt_cast<Bytef *>(cpu::translate(zstrm->next_out));
   wstrm->avail_out = zstrm->avail_out;
   wstrm->total_out = zstrm->total_out;

   wstrm->data_type = zstrm->data_type;
   wstrm->adler = zstrm->adler;

   return result;
}

static int
zlib125_inflateInit_(virt_ptr<zlib125_stream> wstrm,
                     virt_ptr<const char> version,
                     int32_t stream_size)
{
   decaf_check(sizeof(zlib125_stream) == stream_size);

   auto zstrm = getZStream(wstrm);
   auto result = inflateInit_(zstrm, version.getRawPointer(),
                              sizeof(z_stream));

   wstrm->msg = nullptr;
   return result;
}

static int
zlib125_inflateInit2_(virt_ptr<zlib125_stream> wstrm,
                      int32_t windowBits,
                      virt_ptr<const char> version,
                      int32_t stream_size)
{
   decaf_check(sizeof(zlib125_stream) == stream_size);

   auto zstrm = getZStream(wstrm);
   auto result = inflateInit2_(zstrm, windowBits, version.getRawPointer(),
                               sizeof(z_stream));

   wstrm->msg = nullptr;
   return result;
}

static int
zlib125_inflateReset(virt_ptr<zlib125_stream> wstrm)
{
   auto zstrm = getZStream(wstrm);
   return inflateReset(zstrm);
}

static int
zlib125_inflateReset2(virt_ptr<zlib125_stream> wstrm,
                      int32_t windowBits)
{
   auto zstrm = getZStream(wstrm);
   return inflateReset2(zstrm, windowBits);
}

static int
zlib125_inflateEnd(virt_ptr<zlib125_stream> wstrm)
{
   auto zstrm = getZStream(wstrm);
   auto result = inflateEnd(zstrm);
   eraseZStream(wstrm);
   return result;
}

static uint32_t
zlib125_adler32(uint32_t adler,
                virt_ptr<const uint8_t> buf,
                uint32_t len)
{
   return static_cast<uint32_t>(adler32(adler, buf.getRawPointer(), len));
}

static uint32_t
zlib125_crc32(uint32_t crc,
              virt_ptr<const uint8_t> buf,
              uint32_t len)
{
   return static_cast<uint32_t>(crc32(crc, buf.getRawPointer(), len));
}

static int
zlib125_compress(virt_ptr<uint8_t> dest,
                 virt_ptr<uint32_t> destLen,
                 virt_ptr<const uint8_t> source,
                 uint32_t sourceLen)
{
   unsigned long realDestLen = *destLen;
   auto result = compress(dest.getRawPointer(), &realDestLen,
                          source.getRawPointer(), sourceLen);
   *destLen = realDestLen;
   return result;
}

static uint32_t
zlib125_compressBound(uint32_t sourceLen)
{
   return static_cast<uint32_t>(compressBound(sourceLen));
}

static int
zlib125_uncompress(virt_ptr<uint8_t> dest,
                   virt_ptr<uint32_t> destLen,
                   virt_ptr<const uint8_t> source,
                   uint32_t sourceLen)
{
   unsigned long realDestLen = *destLen;
   auto result = uncompress(dest.getRawPointer(), &realDestLen,
                            source.getRawPointer(), sourceLen);
   *destLen = realDestLen;
   return result;
}

static uint32_t
zlib125_zlibCompileFlags()
{
   return static_cast<uint32_t>(zlibCompileFlags());
}

void
Library::registerZlibSymbols()
{
   RegisterFunctionExportName("adler32", zlib125_adler32);
   RegisterFunctionExportName("crc32", zlib125_crc32);
   RegisterFunctionExportName("deflate", zlib125_deflate);
   RegisterFunctionExportName("deflateInit_", zlib125_deflateInit_);
   RegisterFunctionExportName("deflateInit2_", zlib125_deflateInit2_);
   RegisterFunctionExportName("deflateBound", zlib125_deflateBound);
   RegisterFunctionExportName("deflateReset", zlib125_deflateReset);
   RegisterFunctionExportName("deflateEnd", zlib125_deflateEnd);
   RegisterFunctionExportName("inflate", zlib125_inflate);
   RegisterFunctionExportName("inflateInit_", zlib125_inflateInit_);
   RegisterFunctionExportName("inflateInit2_", zlib125_inflateInit2_);
   RegisterFunctionExportName("inflateReset", zlib125_inflateReset);
   RegisterFunctionExportName("inflateReset2", zlib125_inflateReset2);
   RegisterFunctionExportName("inflateEnd", zlib125_inflateEnd);
   RegisterFunctionExportName("compress", zlib125_compress);
   RegisterFunctionExportName("compressBound", zlib125_compressBound);
   RegisterFunctionExportName("uncompress", zlib125_uncompress);
   RegisterFunctionExportName("zlibCompileFlags", zlib125_zlibCompileFlags);
}

} // namespace cafe::zlib125
