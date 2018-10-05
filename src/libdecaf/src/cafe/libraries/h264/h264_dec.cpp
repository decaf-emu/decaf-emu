#include "h264.h"
#include "h264_dec.h"
#include "cafe/libraries/cafe_hle_stub.h"

#include <array>
#include <common/align.h>
#include <common/decaf_assert.h>
#include <fmt/format.h>

#include <fstream>

//#define FFMPEG_DEC/

#ifdef  FFMPEG_DEC
extern "C" 
{
   #include <..\..\..\..\..\libraries\ffmpeg\include\avcodec.h>
   #include <..\..\..\..\..\libraries\ffmpeg\include\avformat.h>
   #include <..\..\..\..\..\libraries\ffmpeg\include\swscale.h>
}
#endif

namespace cafe::h264
{

#ifdef  FFMPEG_DEC
   static AVCodecContext* context;
   static AVFrame* frame;
   static AVPacket avpkt;
#else
   int width;
   int height;
#endif

int32_t
H264DECCheckMemSegmentation(virt_ptr<void> memPtr, 
	                        uint32_t Size)
{
   decaf_warn_stub();
   return 0;
}

int32_t
H264DECMemoryRequirement(int32_t profile,
                         int32_t level,
                         int32_t maxWidth,
                         int32_t maxHeight,
                         virt_ptr<int32_t> codecMemSize)
{
   switch (profile)
   {
   case 66:  // Constrained  Baseline Profile
      break;
   case 77:  // Main Profile
      break;
   case 100: // High Profile
      break;
   default:
      decaf_abort(fmt::format("Unsupported profile {}", profile));
      break;
   }

#ifdef FFMPEG_DEC
   AVCodec *codec = avcodec_find_decoder(AV_CODEC_ID_H264);
   if (!codec) {
      return -1;
   }

   context = avcodec_alloc_context3(codec);
   if (!context) {
      return -1;
   }

   context->bit_rate = 400000;
   context->width = maxWidth;
   context->height = maxHeight;
   context->time_base.num = 1;
   context->time_base.den = 25;
   context->gop_size = 10;
   context->max_b_frames = 1;
   context->pix_fmt = AV_PIX_FMT_YUV420P;
   context->profile = profile;
   context->level = level;

   if (avcodec_open2(context, codec, NULL) < 0) {
      return -1;
   }

   frame = av_frame_alloc();
   if (!frame) {
      return -1;
   }

   frame->width = maxWidth;
   frame->height = maxHeight;
#else
   width = maxWidth;
   height = maxHeight;
#endif
   *codecMemSize = (int32_t)(maxWidth * maxHeight * sizeof(int32_t) * 124);

   return 0;
}

int32_t   
H264DECEnd(virt_ptr<void> memPtr)
{
   decaf_warn_stub();
   return 0;
}


int32_t
H264DECInitParam(int32_t memSize, 
	             virt_ptr<void> memPtr)
{
   decaf_warn_stub();
   return 0;
}

int32_t
H264DECSetParam(virt_ptr<void> memPtr,
                int32_t paramid,
                virt_ptr<void> param)
{
   decaf_warn_stub();
   return 0;
}

int32_t
H264DECOpen(virt_ptr<void> memPtr)
{
   decaf_warn_stub();
   return 0;
}

int32_t
H264DECBegin(virt_ptr<void> memPtr)
{
   decaf_warn_stub();
   return 0;
}

#ifdef FFMPEG_DEC
static int32_t ffmpeg_encoder_init_frame(AVFrame **framep, 
	                                     int width, 
	                                     int height) 
{
   int ret;
   AVFrame *frame = av_frame_alloc();

   if (!frame) {
      gLog->error("Could not allocate video frame");
      return -1;
   }

   frame->format = context->pix_fmt;
   frame->width = width;
   frame->height = height;

   *framep = frame;

   return 0;
}
#endif


int32_t
H264DECExecute(virt_ptr<void> memPtr, 
	           virt_ptr<void> StrFmPtr)
{
#ifdef FFMPEG_DEC
   struct SwsContext *sws;
   sws = sws_getContext(frame->width, frame->height, AV_PIX_FMT_YUV420P, frame->width, frame->height, AV_PIX_FMT_NV12, 2, NULL, NULL, NULL);

   AVFrame* frame2;
   if (ffmpeg_encoder_init_frame(&frame2, frame->width, frame->height)) {
      return -1;
   }

   int num_bytes = avpicture_get_size((AVPixelFormat)(frame->format), frame->width, frame->height);

   if (num_bytes > 0) {
      uint8_t* frame2_buffer = (uint8_t *)av_malloc(num_bytes * sizeof(uint8_t));
      int ret = avpicture_fill((AVPicture*)frame2, frame2_buffer, AV_PIX_FMT_NV12, frame->width, frame->height);

      int height = sws_scale(sws, frame->data, frame->linesize, 0, frame->height, frame2->data, frame2->linesize);

      int num_bytes2 = avpicture_get_size(AV_PIX_FMT_NV12, frame2->width, frame2->height);
      if (num_bytes2 > 0) {
         memcpy(StrFmPtr.getRawPointer(), frame2->data[0], num_bytes2);
      }
   }
#else
   decaf_warn_stub();
   int32_t imageSize = (width * height) + (width * (height / 2));

   if (imageSize > 0) {

      int p = 0;

      uint8_t* d = (uint8_t*)malloc(imageSize);

      uint8_t R = 128;
      uint8_t G = 32;
      uint8_t B = 192;

      uint8_t Y = (0.257 * R) + (0.504 * G) + (0.098 * B) + 16;
      uint8_t V = (0.439 * R) - (0.368 * G) - (0.071 * B) + 128;
      uint8_t U = -(0.148 * R) - (0.291 * G) + (0.439 * B) + 128;

      for (int h = 0; h < height; h++) {
         for (int w = 0; w < width; w++) {
            *(d + p++) = Y;
         }
      }
      for (int h = 0; h < height / 4; h++) {
		  for (int w = 0; w < width; w++) {
			  *(d + p++) = U;
			  *(d + p++) = V;
		  }
	  }

      if (imageSize > 0) {
         memcpy(StrFmPtr.getRawPointer(), d, imageSize);
      }
   }
#endif
   return 0X000000E4;
}



int32_t
H264DECFlush(virt_ptr<void> memPtr)
{
   decaf_warn_stub();
   return 0;
}

int32_t
H264DECClose(virt_ptr<void> memPtr)
{
   decaf_warn_stub();
   return 0;
}

int32_t
H264DECSetBitstream(virt_ptr<void> memPtr,
                    const virt_ptr<uint8_t> bitstream,
                    const int32_t length,
                    const double timeStamp)
{
   int result = 0;

#ifdef FFMPEG_DEC
   avpkt.size = length;
   if (avpkt.size == 0) {
      return -1;
   }

   frame->format = 1;

   avpkt.data = (uint8_t*)std::malloc(length);

   for (int i = 0; i < length; ++i) {
      avpkt.data[i] = bitstream[i];
   }

   int frame_count;
   result = avcodec_decode_video2(context, frame, &frame_count, &avpkt);

   if (result < 0) {
      gLog->error("Error decoding stream");
   }
   else {
      avpkt.size = result;
      result = 0;
   }
#endif

   return result;
}

int32_t
H264DECCheckDecunitLength(virt_ptr<void> memPtr, 
                          const virt_ptr<uint8_t> buf, 
                          int32_t totalBytes, 
                          int32_t streamOffset, 
                          virt_ptr<int32_t>length)
{
   decaf_warn_stub();
   return 0;
}

int32_t 
H264DECCheckSkipableFrame(const virt_ptr<uint8_t> buf, 
                          int32_t Length, 
                          virt_ptr<int32_t>SkipFlag)
{
   decaf_warn_stub();
   return 0;
}

int32_t 
H264DECFindDecstartpoint(const virt_ptr<uint8_t> buf, 
                         int32_t totalBytes, 
                         virt_ptr <int32_t> streamOffset)
{
   decaf_warn_stub();
   return 0;
}

int32_t 
H264DECFindIdrpoint(const virt_ptr <uint8_t> buf, 
                    int32_t totalBytes, 
                    virt_ptr <int32_t> streamOffset)
{
   decaf_warn_stub(); 
   return 0;
}

int32_t 
H264DECGetImageSize(const virt_ptr <uint8_t> buf, 
                    int32_t TotalBytes, 
                    int32_t streamOffset, 
                    virt_ptr<int32_t> Width, 
                    virt_ptr <int32_t> Height)
{
   decaf_warn_stub();
   return 0;
}

void
Library::registerDecSymbols()
{
   RegisterFunctionExport(H264DECBegin);
   RegisterFunctionExport(H264DECCheckDecunitLength);
   RegisterFunctionExport(H264DECCheckMemSegmentation); 
   RegisterFunctionExport(H264DECCheckSkipableFrame);
   RegisterFunctionExport(H264DECClose);
   RegisterFunctionExport(H264DECEnd);
   RegisterFunctionExport(H264DECExecute);
   RegisterFunctionExport(H264DECFindDecstartpoint);
   RegisterFunctionExport(H264DECFindIdrpoint);
   RegisterFunctionExport(H264DECFlush);
   RegisterFunctionExport(H264DECGetImageSize);
   RegisterFunctionExport(H264DECInitParam);
   RegisterFunctionExport(H264DECMemoryRequirement);
   RegisterFunctionExport(H264DECOpen);
   RegisterFunctionExport(H264DECSetBitstream);
   RegisterFunctionExport(H264DECSetParam);
   RegisterFunctionExportName("H264DECSetParam_FPTR_OUTPUT", H264DECSetParam);
   RegisterFunctionExportName("H264DECSetParam_OUTPUT_PER_FRAME", H264DECSetParam);
   RegisterFunctionExportName("H264DECSetParam_USER_MEMORY", H264DECSetParam);
}

} // namespace cafe::h264_dec