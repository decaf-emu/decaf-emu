#include <zlib.h>
#include "utils/wfunc_call.h"
#include "modules/coreinit/coreinit_memheap.h"
#include "memory_translate.h"
#include "processor.h"
#include "utils/virtual_ptr.h"
#include "utils/wfunc_ptr.h"
#include "zlib125.h"

namespace zlib125
{

static std::map<uint32_t, z_stream>
gStreamMap;

// WiiU games will be using a 32bit zlib where stuff is in big endian order in memory
// this means all structures like z_streamp have to be swapped endian to a temp structure

struct WZStream
{
   be_ptr<Bytef> next_in;
   be_val<uInt> avail_in;
   be_val<uLong> total_in;

   be_ptr<Bytef> next_out;
   be_val<uInt> avail_out;
   be_val<uLong> total_out;

   be_ptr<char> msg;
   be_ptr<struct internal_state> state;

   be_ptr<void> zalloc;
   be_ptr<void> zfree;
   be_ptr<void> opaque;

   be_val<int> data_type;
   be_val<uLong> adler;
   be_val<uLong> reserved;
};

struct WZHeader
{
   be_val<int> text;
   be_val<uLong> time;
   be_val<int> xflags;
   be_val<int> os;
   be_ptr<Bytef> extra;
   be_val<uInt> extra_len;
   be_val<uInt> extra_max;
   be_ptr<Bytef> name;
   be_val<uInt> name_max;
   be_ptr<Bytef> comment;
   be_val<uInt> comm_max;
   be_val<int> hcrc;
   be_val<int> done;
};

using ZlibAllocFunc = wfunc_ptr<void*, void*, uint32_t, uint32_t>;
using ZlibFreeFunc = wfunc_ptr<void, void*, void*>;

static void *
zlibAllocWrapper(void *opaque, unsigned items, unsigned size)
{
   auto wstrm = reinterpret_cast<WZStream *>(opaque);
   ZlibAllocFunc allocFunc = wstrm->zalloc;

   if (allocFunc) {
      return allocFunc(wstrm->opaque, items, size);
   } else {
      return coreinit::internal::sysAlloc(items * size);
   }
}

static void
zlibFreeWrapper(void *opaque, void *address)
{
   auto wstrm = reinterpret_cast<WZStream *>(opaque);
   ZlibFreeFunc freeFunc = wstrm->zfree;

   if (freeFunc) {
      freeFunc(wstrm->opaque, address);
   } else {
      coreinit::internal::sysFree(address);
   }
}

z_stream *
getZStream(WZStream *in)
{
   auto zstream = &gStreamMap[memory_untranslate(in)];
   zstream->opaque = in;
   zstream->zalloc = &zlibAllocWrapper;
   zstream->zfree = &zlibFreeWrapper;
   return zstream;
}

void
eraseZStream(WZStream *in)
{
   gStreamMap.erase(memory_untranslate(in));
}

static int
zlib125_deflate(WZStream *wstrm, int flush)
{
   auto zstrm = getZStream(wstrm);
   zstrm->next_in = wstrm->next_in;
   zstrm->avail_in = wstrm->avail_in;
   zstrm->total_in = wstrm->total_in;

   zstrm->next_out = wstrm->next_out;
   zstrm->avail_out = wstrm->avail_out;
   zstrm->total_out = wstrm->total_out;

   zstrm->data_type = wstrm->data_type;
   zstrm->adler = wstrm->adler;

   auto result = deflate(zstrm, flush);

   wstrm->next_in = zstrm->next_in;
   wstrm->avail_in = zstrm->avail_in;
   wstrm->total_in = zstrm->total_in;

   wstrm->next_out = zstrm->next_out;
   wstrm->avail_out = zstrm->avail_out;
   wstrm->total_out = zstrm->total_out;

   wstrm->data_type = zstrm->data_type;
   wstrm->adler = zstrm->adler;

   return result;
}

static int
zlib125_deflateInit_(WZStream *wstrm, int level, const char *version, int stream_size)
{
   assert(sizeof(WZStream) == stream_size);

   auto zstrm = getZStream(wstrm);
   auto result = deflateInit_(zstrm, level, version, sizeof(z_stream));

   wstrm->msg = nullptr;
   return result;
}

static int
zlib125_deflateInit2_(WZStream *wstrm, int level, int method, int windowBits, int memLevel, int strategy, const char *version, int stream_size)
{
   assert(sizeof(WZStream) == stream_size);

   auto zstrm = getZStream(wstrm);
   auto result = deflateInit2_(zstrm, level, method, windowBits, memLevel, strategy, version, sizeof(z_stream));

   wstrm->msg = nullptr;
   return result;
}

static uint32_t
zlib125_deflateBound(WZStream *wstrm, uint32_t sourceLen)
{
   auto zstrm = getZStream(wstrm);
   return deflateBound(zstrm, sourceLen);
}

static int
zlib125_deflateEnd(WZStream *wstrm)
{
   auto zstrm = getZStream(wstrm);
   return deflateEnd(zstrm);
}

static int
zlib125_inflate(WZStream *wstrm, int flush)
{
   auto zstrm = getZStream(wstrm);
   zstrm->next_in = wstrm->next_in;
   zstrm->avail_in = wstrm->avail_in;
   zstrm->total_in = wstrm->total_in;

   zstrm->next_out = wstrm->next_out;
   zstrm->avail_out = wstrm->avail_out;
   zstrm->total_out = wstrm->total_out;

   zstrm->data_type = wstrm->data_type;
   zstrm->adler = wstrm->adler;

   auto result = inflate(zstrm, flush);

   wstrm->next_in = zstrm->next_in;
   wstrm->avail_in = zstrm->avail_in;
   wstrm->total_in = zstrm->total_in;

   wstrm->next_out = zstrm->next_out;
   wstrm->avail_out = zstrm->avail_out;
   wstrm->total_out = zstrm->total_out;

   wstrm->data_type = zstrm->data_type;
   wstrm->adler = zstrm->adler;

   return result;
}

static int
zlib125_inflateInit_(WZStream *wstrm, const char *version, int stream_size)
{
   assert(sizeof(WZStream) == stream_size);

   auto zstrm = getZStream(wstrm);
   auto result = inflateInit_(zstrm, version, sizeof(z_stream));

   wstrm->msg = nullptr;
   return result;
}

static int
zlib125_inflateInit2_(WZStream *wstrm, int windowBits, const char *version, int stream_size)
{
   assert(sizeof(WZStream) == stream_size);

   auto zstrm = getZStream(wstrm);
   auto result = inflateInit2_(zstrm, windowBits, version, sizeof(z_stream));

   wstrm->msg = nullptr;
   return result;
}

static int
zlib125_inflateEnd(WZStream *wstrm)
{
   auto zstrm = getZStream(wstrm);
   auto result = inflateEnd(zstrm);
   eraseZStream(wstrm);
   return result;
}

static uint32_t
zlib125_adler32(uint32_t adler, const Bytef * buf, unsigned len)
{
   return static_cast<uint32_t>(adler32(adler, buf, len));
}

static uint32_t
zlib125_crc32(uint32_t crc, const Bytef * buf, unsigned len)
{
   return static_cast<uint32_t>(crc32(crc, buf, len));
}

static int
zlib125_compress(Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen)
{
   return compress(dest, destLen, source, sourceLen);
}

static uint32_t
zlib125_compressBound(uint32_t sourceLen)
{
   return static_cast<uint32_t>(compressBound(sourceLen));
}

static int
zlib125_uncompress(uint8_t* dest, be_val<uint32_t>* destLen, const uint8_t* source, uint32_t sourceLen)
{
   unsigned long realDestLen = *destLen;
   auto result = uncompress(dest, &realDestLen, source, sourceLen);
   *destLen = realDestLen;
   return result;
}

static uint32_t
zlib125_zlibCompileFlags()
{
   return static_cast<uint32_t>(zlibCompileFlags());
}

void
Module::registerCoreFunctions()
{
   // Need wrap
   RegisterKernelFunctionName("adler32", zlib125_adler32);
   RegisterKernelFunctionName("crc32", zlib125_crc32);
   RegisterKernelFunctionName("deflate", zlib125_deflate);
   RegisterKernelFunctionName("deflateInit_", zlib125_deflateInit_);
   RegisterKernelFunctionName("deflateInit2_", zlib125_deflateInit2_);
   RegisterKernelFunctionName("deflateBound", zlib125_deflateBound);
   RegisterKernelFunctionName("deflateEnd", zlib125_deflateEnd);
   RegisterKernelFunctionName("inflate", zlib125_inflate);
   RegisterKernelFunctionName("inflateInit_", zlib125_inflateInit_);
   RegisterKernelFunctionName("inflateInit2_", zlib125_inflateInit2_);
   RegisterKernelFunctionName("inflateEnd", zlib125_inflateEnd);
   RegisterKernelFunctionName("compress", zlib125_compress);
   RegisterKernelFunctionName("compressBound", zlib125_compressBound);
   RegisterKernelFunctionName("uncompress", zlib125_uncompress);
   RegisterKernelFunctionName("zlibCompileFlags", zlib125_zlibCompileFlags);
}

} // namespace zlib125
