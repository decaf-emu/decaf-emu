#include <zlib.h>
#include "utils/wfunc_call.h"
#include "modules/coreinit/coreinit_memheap.h"
#include "memory_translate.h"
#include "processor.h"
#include "utils/virtual_ptr.h"
#include "utils/wfunc_ptr.h"
#include "zlib125.h"

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
      return OSAllocFromSystem(items * size);
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
      OSFreeToSystem(address);
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
zlib125_inflateInit2_(WZStream *wstrm, int windowBits, const char *version, int stream_size)
{
   assert(sizeof(WZStream) == stream_size);

   auto zstrm = getZStream(wstrm);
   auto result = inflateInit2_(zstrm, windowBits, version, sizeof(z_stream));

   wstrm->msg = nullptr;

   return result;
}

static int
zlib125_inflateInit_(WZStream *wstrm, const char *version, int stream_size)
{
   return zlib125_inflateInit2_(wstrm, 15 /* default window bits*/, version, stream_size);
}

static int
zlib125_inflateEnd(WZStream *wstrm)
{
   auto zstrm = getZStream(wstrm);
   auto result = inflateEnd(zstrm);
   eraseZStream(wstrm);
   return result;
}

void
Zlib125::registerCoreFunctions()
{
   // Functions we can do directly!
   RegisterKernelFunction(adler32);
   RegisterKernelFunction(crc32);
   RegisterKernelFunction(compressBound);
   RegisterKernelFunction(zlibCompileFlags);

   // Need wrap
   RegisterKernelFunctionName("inflate", zlib125_inflate);
   RegisterKernelFunctionName("inflateInit_", zlib125_inflateInit_);
   RegisterKernelFunctionName("inflateInit2_", zlib125_inflateInit2_);
   RegisterKernelFunctionName("inflateEnd", zlib125_inflateEnd);
}
