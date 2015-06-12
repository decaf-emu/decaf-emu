#include "zlib125.h"
#include "zlib125_core.h"

/*
Imported by Super Mario World
int inflate(z_streamp strm, int flush);

int compress(Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen);

int inflateEnd(z_streamp strm);

int inflateInit2_(z_streamp strm, int  windowBits, const char *version, int stream_size);

int uncompress(Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen);
*/


void Zlib125::registerCoreFunctions()
{
}
