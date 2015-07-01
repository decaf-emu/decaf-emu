#include "zlib125.h"
#include "zlib125_core.h"
#include <zlib.h>

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

// Some hack to help with copying between alloc_func/free_func <-> be_val<p32<void>>
template<typename Type1, typename Type2>
static inline void
copy_helper(Type1 &src, Type2 &dst);

template<typename Type2>
static inline void
copy_helper(alloc_func &src, Type2 &dst)
{
   dst = (void*)src;
}

template<typename Type1>
static inline void
copy_helper(Type1 &src, alloc_func &dst)
{
   dst = reinterpret_cast<alloc_func>(static_cast<void*>(src));
}

template<typename Type2>
static inline void
copy_helper(free_func &src, Type2 &dst)
{
   dst = (void*)src;
}

template<typename Type1>
static inline void
copy_helper(Type1 &src, free_func &dst)
{
   dst = reinterpret_cast<free_func>(static_cast<void*>(src));
}

template<typename Type1, typename Type2>
static inline void
copy_helper(Type1 &src, Type2 &dst)
{
   dst = src;
}

template<typename SrcType, typename DstType>
static void
swapZStream(SrcType *src, DstType *dst)
{
   // TODO: Support these callbacks from zlib into game code...
   assert(src->zalloc == nullptr);
   assert(src->zfree == nullptr);
   assert(src->opaque == nullptr);

   dst->next_in = src->next_in;
   dst->avail_in = src->avail_in;
   dst->total_in = src->total_in;

   dst->next_out = src->next_out;
   dst->avail_out = src->avail_out;
   dst->total_out = src->total_out;

   dst->msg = src->msg;
   dst->state = src->state;

   copy_helper(src->zalloc, dst->zalloc);
   copy_helper(src->zfree, dst->zfree);
   dst->opaque = src->opaque;

   dst->data_type = src->data_type;
   dst->adler = src->adler;
   dst->reserved = src->reserved;
}

template<typename SrcType, typename DstType>
static void
swapZHeader(SrcType *src, DstType *dst)
{
   dst->text = src->text;
   dst->time = src->time;
   dst->xflags = src->xflags;
   dst->os = src->os;
   dst->extra = src->extra;
   dst->extra_len = src->extra_len;
   dst->extra_max = src->extra_max;
   dst->name = src->name;
   dst->name_max = src->name_max;
   dst->comment = src->comment;
   dst->comm_max = src->comm_max;
   dst->hcrc = src->hcrc;
   dst->done = src->done;
}

static int
zlib125_compress(Bytef *dest, be_val<uLongf> *destLen, const Bytef *source, uLong sourceLen)
{
   uLongf dstLen = *destLen;
   auto result = compress(dest, &dstLen, source, sourceLen);
   *destLen = dstLen;
   return result;
}

static int
zlib125_compress2(Bytef *dest, be_val<uLongf> *destLen, const Bytef *source, uLong sourceLen, int level)
{
   uLongf dstLen = *destLen;
   auto result = compress2(dest, &dstLen, source, sourceLen, level);
   *destLen = dstLen;
   return result;
}

static int
zlib125_deflate(WZStream *wstrm, int flush)
{
   z_stream zstrm;
   swapZStream(wstrm, &zstrm);
   auto result = deflate(&zstrm, flush);
   swapZStream(&zstrm, wstrm);
   return result;
}

static uLong
zlib125_deflateBound(WZStream *strm, uLong sourceLen)
{
   z_stream zstrm;
   swapZStream(strm, &zstrm);
   auto result = deflateBound(&zstrm, sourceLen);
   swapZStream(&zstrm, strm);
   return result;
}

static int
zlib125_deflateCopy(WZStream *dst, WZStream *src)
{
   z_stream wdst, wsrc;
   swapZStream(dst, &wdst);
   swapZStream(src, &wsrc);
   auto result = deflateCopy(&wdst, &wsrc);
   swapZStream(&wdst, dst);
   swapZStream(&wsrc, src);
   return result;
}

static int
zlib125_deflateEnd(WZStream *strm)
{
   z_stream zstrm;
   swapZStream(strm, &zstrm);
   auto result = deflateEnd(&zstrm);
   swapZStream(&zstrm, strm);
   return result;
}

static int
zlib125_deflateInit_(WZStream *strm, int level, const char *version, int stream_size)
{
   z_stream zstrm;
   swapZStream(strm, &zstrm);
   auto result = deflateInit_(&zstrm, level, version, stream_size);
   swapZStream(&zstrm, strm);
   return result;
}

static int
zlib125_deflateInit2_(WZStream *strm, int  level, int  method, int windowBits, int memLevel, int strategy, const char *version, int stream_size)
{
   z_stream zstrm;
   swapZStream(strm, &zstrm);
   auto result = deflateInit2_(&zstrm, level, method, windowBits, memLevel, strategy, version, stream_size);
   swapZStream(&zstrm, strm);
   return result;
}

static int
zlib125_deflateParams(WZStream *wstrm, int level, int strategy)
{
   z_stream zstrm;
   swapZStream(wstrm, &zstrm);
   auto result = deflateParams(&zstrm, level, strategy);
   swapZStream(&zstrm, wstrm);
   return result;
}

static int
zlib125_deflatePrime(WZStream *wstrm, int bits, int value)
{
   z_stream zstrm;
   swapZStream(wstrm, &zstrm);
   auto result = deflateParams(&zstrm, bits, value);
   swapZStream(&zstrm, wstrm);
   return result;
}

static int
zlib125_deflateReset(WZStream *wstrm)
{
   z_stream zstrm;
   swapZStream(wstrm, &zstrm);
   auto result = deflateReset(&zstrm);
   swapZStream(&zstrm, wstrm);
   return result;
}

static int
zlib125_deflateSetDictionary(WZStream *wstrm, const Bytef *dictionary, uInt dictLength)
{
   z_stream zstrm;
   swapZStream(wstrm, &zstrm);
   auto result = deflateSetDictionary(&zstrm, dictionary, dictLength);
   swapZStream(&zstrm, wstrm);
   return result;
}

static int
zlib125_deflateSetHeader(WZStream *wstrm, WZHeader *head)
{
   z_stream zstrm;
   gz_header zhdr;
   swapZHeader(head, &zhdr);
   swapZStream(wstrm, &zstrm);
   auto result = deflateSetHeader(&zstrm, &zhdr);
   swapZStream(&zstrm, wstrm);
   swapZHeader(&zhdr, head);
   return result;
}

static int
zlib125_deflateTune(WZStream *wstrm, int good_length, int max_lazy, int nice_length, int max_chain)
{
   z_stream zstrm;
   swapZStream(wstrm, &zstrm);
   auto result = deflateTune(&zstrm, good_length, max_lazy, nice_length, max_chain);
   swapZStream(&zstrm, wstrm);
   return result;
}

static int
zlib125_inflate(WZStream *wstrm, int flush)
{
   z_stream zstrm;
   swapZStream(wstrm, &zstrm);
   auto result = inflate(&zstrm, flush);
   swapZStream(&zstrm, wstrm);
   return result;
}

static int
zlib125_inflateBack(WZStream *wstrm, in_func in, void FAR *in_desc, out_func out, void FAR *out_desc)
{
   z_stream zstrm;

   // TODO: Support these callbacks from zlib into gamecode...
   assert(in == nullptr);
   assert(in_desc == nullptr);
   assert(out == nullptr);
   assert(out_desc == nullptr);

   swapZStream(wstrm, &zstrm);
   auto result = inflateBack(&zstrm, in, in_desc, out, out_desc);
   swapZStream(&zstrm, wstrm);
   return result;
}

static int
zlib125_inflateBackEnd(WZStream *wstrm)
{
   z_stream zstrm;
   swapZStream(wstrm, &zstrm);
   auto result = inflateBackEnd(&zstrm);
   swapZStream(&zstrm, wstrm);
   return result;
}

static int
zlib125_inflateBackInit_(WZStream *wstrm, int windowBits, unsigned char FAR *window, const char *version, int stream_size)
{
   z_stream zstrm;
   swapZStream(wstrm, &zstrm);
   auto result = inflateBackInit_(&zstrm, windowBits, window, version, stream_size);
   swapZStream(&zstrm, wstrm);
   return result;
}

static int
zlib125_inflateCopy(WZStream *dst, WZStream *src)
{
   z_stream wdst, wsrc;
   swapZStream(dst, &wdst);
   swapZStream(src, &wsrc);
   auto result = inflateCopy(&wdst, &wsrc);
   swapZStream(&wdst, dst);
   swapZStream(&wsrc, src);
   return result;
}

static int
zlib125_inflateEnd(WZStream *wstrm)
{
   z_stream zstrm;
   swapZStream(wstrm, &zstrm);
   auto result = inflateEnd(&zstrm);
   swapZStream(&zstrm, wstrm);
   return result;
}

static int
zlib125_inflateGetHeader(WZStream *wstrm, WZHeader *head)
{
   z_stream zstrm;
   gz_header zhdr;
   swapZHeader(head, &zhdr);
   swapZStream(wstrm, &zstrm);
   auto result = inflateGetHeader(&zstrm, &zhdr);
   swapZStream(&zstrm, wstrm);
   swapZHeader(&zhdr, head);
   return result;
}

static int
zlib125_inflateInit2_(WZStream *wstrm, int windowBits, const char *version, int stream_size)
{
   z_stream zstrm;
   swapZStream(wstrm, &zstrm);
   auto result = inflateInit2_(&zstrm, windowBits, version, stream_size);
   swapZStream(&zstrm, wstrm);
   return result;
}

static int
zlib125_inflateInit_(WZStream *wstrm, const char *version, int stream_size)
{
   z_stream zstrm;
   swapZStream(wstrm, &zstrm);
   auto result = inflateInit_(&zstrm, version, stream_size);
   swapZStream(&zstrm, wstrm);
   return result;
}

static long
zlib125_inflateMark(WZStream *wstrm)
{
   z_stream zstrm;
   swapZStream(wstrm, &zstrm);
   auto result = inflateMark(&zstrm);
   swapZStream(&zstrm, wstrm);
   return result;
}

static int
zlib125_inflatePrime(WZStream *wstrm, int bits, int value)
{
   z_stream zstrm;
   swapZStream(wstrm, &zstrm);
   auto result = inflatePrime(&zstrm, bits, value);
   swapZStream(&zstrm, wstrm);
   return result;
}

static int
zlib125_inflateReset(WZStream *wstrm)
{
   z_stream zstrm;
   swapZStream(wstrm, &zstrm);
   auto result = inflateReset(&zstrm);
   swapZStream(&zstrm, wstrm);
   return result;
}

static int
zlib125_inflateReset2(WZStream *wstrm, int windowBits)
{
   z_stream zstrm;
   swapZStream(wstrm, &zstrm);
   auto result = inflateReset2(&zstrm, windowBits);
   swapZStream(&zstrm, wstrm);
   return result;
}

static int
zlib125_inflateSync(WZStream *wstrm)
{
   z_stream zstrm;
   swapZStream(wstrm, &zstrm);
   auto result = inflateSync(&zstrm);
   swapZStream(&zstrm, wstrm);
   return result;
}

static int
zlib125_inflateSetDictionary(WZStream *wstrm, const Bytef *dictionary, uInt dictLength)
{
   z_stream zstrm;
   swapZStream(wstrm, &zstrm);
   auto result = inflateSetDictionary(&zstrm, dictionary, dictLength);
   swapZStream(&zstrm, wstrm);
   return result;
}

static int
zlib125_uncompress(Bytef *dest, be_val<uLongf> *destLen, const Bytef *source, uLong sourceLen)
{
   uLongf dstLen = *destLen;
   auto result = uncompress(dest, &dstLen, source, sourceLen);
   *destLen = dstLen;
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
   RegisterKernelFunctionName("compress", zlib125_compress);
   RegisterKernelFunctionName("compress2", zlib125_compress2);
   RegisterKernelFunctionName("deflate", zlib125_deflate);
   RegisterKernelFunctionName("deflateBound", zlib125_deflateBound);
   RegisterKernelFunctionName("deflateCopy", zlib125_deflateCopy);
   RegisterKernelFunctionName("deflateEnd", zlib125_deflateEnd);
   RegisterKernelFunctionName("deflateInit2_", zlib125_deflateInit2_);
   RegisterKernelFunctionName("deflateInit_", zlib125_deflateInit_);
   RegisterKernelFunctionName("deflateParams", zlib125_deflateParams);
   RegisterKernelFunctionName("deflatePrime", zlib125_deflatePrime);
   RegisterKernelFunctionName("deflateReset", zlib125_deflateReset);
   RegisterKernelFunctionName("deflateSetDictionary", zlib125_deflateSetDictionary);
   RegisterKernelFunctionName("deflateSetHeader", zlib125_deflateSetHeader);
   RegisterKernelFunctionName("deflateTune", zlib125_deflateTune);
   RegisterKernelFunctionName("inflate", zlib125_inflate);
   RegisterKernelFunctionName("inflateBack", zlib125_inflateBack);
   RegisterKernelFunctionName("inflateBackEnd", zlib125_inflateBackEnd);
   RegisterKernelFunctionName("inflateBackInit_", zlib125_inflateBackInit_);
   RegisterKernelFunctionName("inflateCopy", zlib125_inflateCopy);
   RegisterKernelFunctionName("inflateEnd", zlib125_inflateEnd);
   RegisterKernelFunctionName("inflateGetHeader", zlib125_inflateGetHeader);
   RegisterKernelFunctionName("inflateInit2_", zlib125_inflateInit2_);
   RegisterKernelFunctionName("inflateInit_", zlib125_inflateInit_);
   RegisterKernelFunctionName("inflateMark", zlib125_inflateMark);
   RegisterKernelFunctionName("inflatePrime", zlib125_inflatePrime);
   RegisterKernelFunctionName("inflateReset", zlib125_inflateReset);
   RegisterKernelFunctionName("inflateReset2", zlib125_inflateReset2);
   RegisterKernelFunctionName("inflateSetDictionary", zlib125_inflateSetDictionary);
   RegisterKernelFunctionName("inflateSync", zlib125_inflateSync);
   RegisterKernelFunctionName("uncompress", zlib125_uncompress);
}
