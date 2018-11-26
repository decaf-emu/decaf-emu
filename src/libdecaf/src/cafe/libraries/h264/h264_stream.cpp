#include "h264.h"
#include "h264_bitstream.h"
#include "h264_decode.h"
#include "h264_enum.h"
#include "h264_stream.h"

#include "cafe/cafe_stackobject.h"

namespace cafe::h264
{

namespace internal
{

static void
clearPictureParameterSet(virt_ptr<H264PictureParameterSet> pps)
{
   std::memset(pps.get(), 0, sizeof(H264PictureParameterSet));
   pps->scalingList4x4.fill(16);
   pps->scalingList8x8.fill(16);
}

static void
clearSequenceParameterSet(virt_ptr<H264SequenceParameterSet> sps)
{
   std::memset(sps.get(), 0, sizeof(H264SequenceParameterSet));
   sps->scalingList4x4.fill(16);
   sps->scalingList8x8.fill(16);
}

static bool
rbspHasMoreData(BitStream &bs)
{
   if (bs.eof()) {
      return false;
   }

   // No rbsp_stop_bit yet
   if (bs.peekU1() == 0) {
      return true;
   }

   // Next bit is 1, is it the rsbp_stop_bit? only if the rest of bits are 0
   auto bsTmp = BitStream { bs };
   bsTmp.readU1();
   while (!bsTmp.eof()) {
      // A later bit was 1, it wasn't the rsbp_stop_bit
      if (bsTmp.readU1() == 1) {
         return true;
      }
   }

   // All following bits were 0, it was the rsbp_stop_bit
   return false;
}

static void
rbspReadTrailingBits(BitStream &bs)
{
   // Read rbsp_stop_one_bit
   bs.readU1();

   // Read rbsp_alignment_zero_bit until we are aligned
   while (!bs.byteAligned()) {
      bs.readU1();
   }
}

static void
readScalingList(BitStream &bs,
                int8_t *scalingList,
                int sizeOfScalingList)
{
   int lastScale = 8;
   int nextScale = 8;

   for (int i = 0; i < sizeOfScalingList; ++i) {
      if (nextScale != 0) {
         nextScale = (lastScale + bs.readSE() + 256) % 256;
      }

      scalingList[i] = (nextScale == 0) ? lastScale : nextScale;
      lastScale = scalingList[i];
   }
}

static void
readHrdParameters(BitStream &bs,
                  virt_ptr<H264SequenceParameterSet> sps)
{
   sps->hrd_cpb_cnt_minus1 = static_cast<uint16_t>(bs.readUE());
   if (sps->hrd_cpb_cnt_minus1 < 32) {
      sps->hrd_bit_rate_scale = bs.readU4();
      sps->hrd_cpb_size_scale = bs.readU4();

      for (auto i = 0; i <= sps->hrd_cpb_cnt_minus1; ++i) {
         sps->hrd_bit_rate_value_minus1[i] = static_cast<uint16_t>(bs.readUE());
         sps->hrd_cpb_size_value_minus1[i] = static_cast<uint16_t>(bs.readUE());
         sps->hrd_cbr_flag[i] = bs.readU1();
      }

      sps->hrd_initial_cpb_removal_delay_length_minus1 = bs.readU5();
      sps->hrd_cpb_removal_delay_length_minus1 = bs.readU5();
      sps->hrd_dpb_output_delay_length_minus1 = bs.readU5();
      sps->hrd_time_offset_length = bs.readU5();
   }
}

static void
readVuiParameters(BitStream &bs,
                  virt_ptr<H264SequenceParameterSet> sps)
{
   sps->vui_aspect_ratio_info_present_flag = bs.readU1();
   if (sps->vui_aspect_ratio_info_present_flag) {
      sps->vui_aspect_ratio_idc = bs.readU8();
      if (sps->vui_aspect_ratio_idc == SarExtended) {
         sps->vui_sar_width = bs.readU16();
         sps->vui_sar_height = bs.readU16();
      }
   }

   sps->vui_overscan_info_present_flag = bs.readU1();
   if (sps->vui_overscan_info_present_flag) {
      sps->vui_overscan_appropriate_flag = bs.readU1();
   }

   sps->vui_video_signal_type_present_flag = bs.readU1();
   if (sps->vui_video_signal_type_present_flag) {
      sps->vui_video_format = bs.readU3();
      sps->vui_video_full_range_flag = bs.readU1();
      sps->vui_colour_description_present_flag = bs.readU1();

      if (sps->vui_colour_description_present_flag) {
         sps->vui_colour_primaries = bs.readU8();
         sps->vui_transfer_characteristics = bs.readU8();
         sps->vui_matrix_coefficients = bs.readU8();
      }
   }

   sps->vui_chroma_loc_info_present_flag = bs.readU1();
   if (sps->vui_chroma_loc_info_present_flag) {
      sps->vui_chroma_sample_loc_type_top_field = static_cast<uint8_t>(bs.readUE());
      sps->vui_chroma_sample_loc_type_bottom_field = static_cast<uint8_t>(bs.readUE());
   }

   sps->vui_timing_info_present_flag = bs.readU1();
   if (sps->vui_timing_info_present_flag) {
      sps->vui_num_units_in_tick = bs.readU(32);
      sps->vui_time_scale = bs.readU(32);
      sps->vui_fixed_frame_rate_flag = bs.readU1();
   }

   sps->vui_nal_hrd_parameters_present_flag = bs.readU1();
   if (sps->vui_nal_hrd_parameters_present_flag) {
      sps->unk0x95E = uint8_t { 1 };
      readHrdParameters(bs, sps);
   }

   sps->vui_vcl_hrd_parameters_present_flag = bs.readU1();
   if (sps->vui_vcl_hrd_parameters_present_flag) {
      sps->unk0x95F = uint8_t { 1 };
      readHrdParameters(bs, sps);
   }

   if (sps->vui_nal_hrd_parameters_present_flag || sps->vui_vcl_hrd_parameters_present_flag) {
      sps->unk0x960 = uint8_t { 1 };
      sps->vui_low_delay_hrd_flag = bs.readU1();
   }

   sps->vui_pic_struct_present_flag = bs.readU1();
   sps->vui_bitstream_restriction_flag = bs.readU1();
   if (sps->vui_bitstream_restriction_flag) {
      sps->vui_motion_vectors_over_pic_boundaries_flag = bs.readU1();
      sps->vui_max_bytes_per_pic_denom = static_cast<uint16_t>(bs.readUE());
      sps->vui_max_bits_per_mb_denom = static_cast<uint16_t>(bs.readUE());
      sps->vui_log2_max_mv_length_horizontal = static_cast<uint16_t>(bs.readUE());
      sps->vui_log2_max_mv_length_vertical = static_cast<uint16_t>(bs.readUE());
      sps->vui_num_reorder_frames = static_cast<uint16_t>(bs.readUE());
      sps->unk0x7B0 = uint16_t { 5 };
      sps->vui_max_dec_frame_buffering = static_cast<uint16_t>(bs.readUE() + 1);
   }
}

static H264Error
readSequenceParameterSet(virt_ptr<H264StreamMemory> streamMemory,
                         const uint8_t *buffer,
                         uint32_t bufferSize,
                         virt_ptr<H264SequenceParameterSet> outSps,
                         size_t *outBytesRead)
{
   StackObject<H264SequenceParameterSet> sps;
   auto bs = BitStream { buffer, bufferSize };
   sps->profile_idc = bs.readU8();
   if (sps->profile_idc != 66 &&
       sps->profile_idc != 77 &&
       sps->profile_idc != 100) {
      return H264Error::InvalidProfile;
   }

   sps->constraint_set = bs.readU8();
   sps->level_idc = bs.readU8();
   sps->seq_parameter_set_id = static_cast<uint8_t>(bs.readUE());
   if (sps->seq_parameter_set_id >= MaxSeqParameterSets) {
      return H264Error::InvalidSps;
   }

   if (sps->profile_idc == 100 || sps->profile_idc == 110 ||
       sps->profile_idc == 122 || sps->profile_idc == 144) {
      sps->chroma_format_idc = static_cast<uint8_t>(bs.readUE());
      if (sps->chroma_format_idc == 3) {
         sps->residual_colour_transform_flag = bs.readU1();
      }

      sps->bit_depth_luma_minus8 = static_cast<uint8_t>(bs.readUE());
      sps->bit_depth_chroma_minus8 = static_cast<uint8_t>(bs.readUE());
      sps->qpprime_y_zero_transform_bypass_flag = bs.readU1();
      sps->seq_scaling_matrix_present_flag = bs.readU1();

      if (sps->seq_scaling_matrix_present_flag) {
         for (auto i = 0u; i < 8; ++i) {
            auto seq_scaling_list_present_flag = bs.readU1();
            if (seq_scaling_list_present_flag) {
               if (i < 6) {
                  readScalingList(bs, (virt_addrof(sps->scalingList4x4) + 16 * i).get(), 16);
               } else {
                  readScalingList(bs, (virt_addrof(sps->scalingList8x8) + 64 * i).get(), 64);
               }
            }
         }
      }
   }

   sps->log2_max_frame_num_minus4 = static_cast<uint16_t>(bs.readUE());
   if (sps->log2_max_frame_num_minus4 > 12) {
      return H264Error::InvalidSps;
   }

   sps->log2_max_frame_num = static_cast<uint8_t>(sps->log2_max_frame_num_minus4 + 4);
   sps->max_frame_num = 1u << sps->log2_max_frame_num;

   sps->pic_order_cnt_type = static_cast<uint16_t>(bs.readUE());
   if (sps->pic_order_cnt_type > 2) {
      return H264Error::InvalidSps;
   }

   if (sps->pic_order_cnt_type == 0) {
      sps->log2_max_pic_order_cnt_lsb_minus4 = static_cast<uint16_t>(bs.readUE());
      sps->max_pic_order_cnt_lsb = 1u << (sps->log2_max_pic_order_cnt_lsb_minus4 + 4);
   } else if (sps->pic_order_cnt_type == 1) {
      sps->delta_pic_order_always_zero_flag = bs.readU1();
      sps->offset_for_non_ref_pic = bs.readSE();
      sps->offset_for_top_to_bottom_field = bs.readSE();
      sps->num_ref_frames_in_pic_order_cnt_cycle = static_cast<uint16_t>(bs.readUE());

      for (int i = 0; i < sps->num_ref_frames_in_pic_order_cnt_cycle; ++i) {
         sps->offset_for_ref_frame[i] = bs.readSE();
      }
   }

   sps->num_ref_frames = static_cast<uint16_t>(bs.readUE());
   if (sps->num_ref_frames > 16) {
      return H264Error::InvalidSps;
   }

   sps->gaps_in_frame_num_value_allowed_flag = bs.readU1();
   sps->pic_width_in_mbs = static_cast<uint16_t>(bs.readUE() + 1);
   sps->pic_width = static_cast<uint16_t>(16 * sps->pic_width_in_mbs);
   sps->pic_height_in_map_units = static_cast<uint16_t>(bs.readUE() + 1);
   sps->pic_height = static_cast<uint16_t>(16 * sps->pic_height_in_map_units);
   sps->pic_size_in_mbs = static_cast<uint16_t>(sps->pic_width_in_mbs * sps->pic_height_in_map_units);
   sps->frame_mbs_only_flag = bs.readU1();
   if (!sps->frame_mbs_only_flag) {
      sps->mb_adaptive_frame_field_flag = bs.readU1();
   }
   sps->unk0x7B8 = static_cast<uint16_t>((2 - sps->frame_mbs_only_flag) * sps->pic_height_in_map_units);

   sps->direct_8x8_inference_flag = bs.readU1();
   sps->frame_cropping_flag = bs.readU1();
   if (sps->frame_cropping_flag) {
      sps->frame_crop_left_offset = static_cast<uint16_t>(bs.readUE());
      sps->frame_crop_right_offset = static_cast<uint16_t>(bs.readUE());
      sps->frame_crop_top_offset = static_cast<uint16_t>(bs.readUE());
      sps->frame_crop_bottom_offset = static_cast<uint16_t>(bs.readUE());
   }

   if (sps->level_idc > 51) {
      sps->level_idc = uint16_t { 51 };
   }

   sps->unk0x7B0 = uint16_t { 5 };
   sps->vui_parameters_present_flag = bs.readU1();
   if (sps->vui_parameters_present_flag) {
      readVuiParameters(bs, sps);
   }

   if (sps->unk0x7B0 > 5) {
      sps->unk0x7B0 = uint16_t { 5 };
   }

   rbspReadTrailingBits(bs);

   sps->valid = uint8_t { 1 };
   sps->unk0x7CC = static_cast<uint16_t>(sps->pic_width_in_mbs * sps->unk0x7B8);
   sps->unk0x7C6 = static_cast<uint16_t>(sps->pic_width_in_mbs * sps->pic_height_in_map_units);

   if (outSps) {
      *outSps = *sps;
   } else {
      streamMemory->spsTable[sps->seq_parameter_set_id] = *sps;
   }

   if (outBytesRead) {
      *outBytesRead = bs.bytesRead();
   }

   return H264Error::OK;
}

static H264Error
readPictureParameterSet(virt_ptr<H264StreamMemory> streamMemory,
                        const uint8_t *buffer,
                        uint32_t bufferSize,
                        size_t *outBytesRead)
{
   auto bs = BitStream { buffer, bufferSize };
   auto pic_parameter_set_id = bs.readUE();
   if (pic_parameter_set_id >= MaxPicParameterSets) {
      return H264Error::InvalidPps;
   }

   auto pps = virt_addrof(streamMemory->ppsTable[pic_parameter_set_id]);
   clearPictureParameterSet(pps);

   pps->seq_parameter_set_id = static_cast<uint16_t>(bs.readUE());
   if (pps->seq_parameter_set_id >= MaxSeqParameterSets) {
      return H264Error::InvalidPps;
   }

   pps->entropy_coding_mode_flag = bs.readU1();
   pps->pic_order_present_flag = bs.readU1();
   pps->num_slice_groups_minus1 = static_cast<uint16_t>(bs.readUE());
   if (pps->num_slice_groups_minus1 > 7) {
      return H264Error::InvalidPps;
   }

   if (pps->num_slice_groups_minus1 > 0) {
      pps->slice_group_map_type = static_cast<uint16_t>(bs.readUE());
      if (pps->slice_group_map_type == 0) {
         for (int i = 0u; i <= pps->num_slice_groups_minus1; ++i) {
            pps->run_length_minus1[i] = static_cast<uint16_t>(bs.readUE());
         }
      } else if (pps->slice_group_map_type == 2) {
         for (auto i = 0u; i < pps->num_slice_groups_minus1; i++) {
            pps->top_left[i] = static_cast<uint16_t>(bs.readUE());
            pps->bottom_right[i] = static_cast<uint16_t>(bs.readUE());
         }
      } else if (pps->slice_group_map_type == 3 ||
                 pps->slice_group_map_type == 4 ||
                 pps->slice_group_map_type == 5) {
         pps->slice_group_change_direction_flag = bs.readU1();
         pps->slice_group_change_rate_minus1 = static_cast<uint16_t>(bs.readUE());
      } else if (pps->slice_group_map_type == 6) {
         pps->pic_size_in_map_units_minus1 = static_cast<uint16_t>(bs.readUE());
         if (pps->pic_size_in_map_units_minus1 > 30800) {
            return H264Error::InvalidPps;
         }

         auto sliceGroupIdNumBits = static_cast<size_t>(std::log2(pps->num_slice_groups_minus1 + 1));
         for (int i = 0; i <= pps->pic_size_in_map_units_minus1; i++) {
            // pps->slice_group_id
            bs.readU(sliceGroupIdNumBits);
         }
      }
   }
   pps->num_ref_idx_l0_active = static_cast<uint8_t>(bs.readUE() + 1);
   if (pps->num_ref_idx_l0_active > 31) {
      return H264Error::InvalidPps;
   }

   pps->num_ref_idx_l1_active = static_cast<uint8_t>(bs.readUE() + 1);
   if (pps->num_ref_idx_l1_active > 31) {
      return H264Error::InvalidPps;
   }

   pps->weighted_pred_flag = bs.readU1();
   pps->weighted_bipred_idc = bs.readU2();
   pps->pic_init_qp_minus26 = static_cast<int16_t>(bs.readSE());
   pps->pic_init_qs_minus26 = static_cast<int16_t>(bs.readSE());
   pps->chroma_qp_index_offset = static_cast<int16_t>(bs.readSE());
   pps->deblocking_filter_control_present_flag = bs.readU1();
   pps->constrained_intra_pred_flag = bs.readU1();
   pps->redundant_pic_cnt_present_flag = bs.readU1();

   auto &sps = streamMemory->spsTable[pps->seq_parameter_set_id];
   if ((sps.profile_idc == 100 || sps.profile_idc == 110 ||
        sps.profile_idc == 122 || sps.profile_idc == 144)
       && rbspHasMoreData(bs)) {
      pps->transform_8x8_mode_flag = bs.readU1();
      pps->pic_scaling_matrix_present_flag = bs.readU1();

      if (pps->pic_scaling_matrix_present_flag) {
         for (auto i = 0; i < 6 + 2 * pps->transform_8x8_mode_flag; i++) {
            pps->pic_scaling_list_present_flag[i] = bs.readU1();
            if (pps->pic_scaling_list_present_flag[i]) {
               if (i < 6) {
                  readScalingList(bs, (virt_addrof(pps->scalingList4x4) + 16 * i).get(), 16);
               } else {
                  readScalingList(bs, (virt_addrof(pps->scalingList8x8) + 64 * i).get(), 64);
               }
            }
         }
      }

      pps->second_chroma_qp_index_offset = static_cast<int16_t>(bs.readSE());
   } else {
      pps->second_chroma_qp_index_offset = pps->chroma_qp_index_offset;
   }

   pps->valid = uint8_t { 1 };
   rbspReadTrailingBits(bs);
   streamMemory->ppsTable[pic_parameter_set_id] = *pps;

   if (outBytesRead) {
      *outBytesRead = bs.bytesRead();
   }

   return H264Error::OK;
}

static uint8_t
decodeSliceType(uint32_t sliceType)
{
   if (sliceType <= 2u) {
      return static_cast<uint8_t>(sliceType);
   } else if (sliceType >= 5u && sliceType <= 7u) {
      return static_cast<uint8_t>(sliceType - 5);
   }

   return 0;
}

static H264Error
readSliceHeader(virt_ptr<H264StreamMemory> streamMemory,
                NaluType naluType,
                const uint8_t *buffer,
                uint32_t bufferSize,
                virt_ptr<H264SliceHeader> slice,
                size_t *outBytesRead)
{
   auto bs = BitStream { buffer, bufferSize };
   slice->field_02 = uint8_t { 1 };
   slice->first_mb_in_slice = static_cast<uint16_t>(bs.readUE());
   slice->slice_type = decodeSliceType(bs.readUE());
   slice->pic_parameter_set_id = static_cast<uint16_t>(bs.readUE());
   if (slice->pic_parameter_set_id >= MaxPicParameterSets) {
      return H264Error::InvalidSliceHeader;
   }

   auto &pps = streamMemory->ppsTable[slice->pic_parameter_set_id];
   if (!pps.valid) {
      return H264Error::InvalidSliceHeader;
   }

   auto &sps = streamMemory->spsTable[pps.seq_parameter_set_id];
   if (!sps.valid) {
      return H264Error::InvalidSliceHeader;
   }

   streamMemory->currentSps = sps;
   streamMemory->currentPps = pps;

   // TODO: More error checking...

   slice->frame_num = static_cast<uint16_t>(bs.readU(sps.log2_max_frame_num));

   if (!sps.frame_mbs_only_flag) {
      slice->field_pic_flag = bs.readU1();

      if (slice->field_pic_flag) {
         slice->bottom_field_flag = bs.readU1();
      }
   }

   if (naluType == NaluType::Idr) {
      slice->idr_pic_id = bs.readUE();
   }

   if (sps.pic_order_cnt_type == 0) {
      slice->pic_order_cnt_lsb = bs.readU(sps.log2_max_pic_order_cnt_lsb_minus4 + 4);

      if (pps.pic_order_present_flag && !slice->field_pic_flag) {
         slice->delta_pic_order_cnt_bottom = bs.readSE();
      } else {
         slice->delta_pic_order_cnt_bottom = 0;
      }
   }

   if (sps.pic_order_cnt_type == 1 && !sps.delta_pic_order_always_zero_flag) {
      slice->delta_pic_order_cnt[0] = bs.readSE();

      if (pps.pic_order_present_flag && !slice->field_pic_flag) {
         slice->delta_pic_order_cnt[1] = bs.readSE();
      }
   }

   if (pps.redundant_pic_cnt_present_flag) {
      slice->redundant_pic_cnt = static_cast<uint16_t>(bs.readUE());
      if (slice->redundant_pic_cnt > 137) {
         return H264Error::InvalidSliceHeader;
      }
   }

   if (slice->slice_type == SliceType::B) {
      slice->direct_spatial_mv_pred_flag = bs.readU1();
   }

   if (slice->slice_type == SliceType::P || slice->slice_type == SliceType::B) {
      slice->num_ref_idx_active_override_flag = bs.readU1();
      if (slice->num_ref_idx_active_override_flag) {
         slice->num_ref_idx_l0_active = static_cast<uint8_t>(bs.readUE() + 1);
         if (slice->num_ref_idx_l0_active > 31) {
            return H264Error::InvalidSliceHeader;
         }

         if (slice->num_ref_idx_l0_active) {
            slice->num_ref_idx_l1_active = static_cast<uint8_t>(bs.readUE() + 1);

            if (slice->num_ref_idx_l1_active > 31) {
               return H264Error::InvalidSliceHeader;
            }
         }
      }
   }

   *outBytesRead = bs.bytesRead();
   return H264Error::OK;
}

/**
 * Find and decode SPS from a NALU stream.
 */
H264Error
decodeNaluSps(const uint8_t *buffer,
              int bufferLength,
              int offset,
              virt_ptr<H264SequenceParameterSet> sps)
{
   // Find SPS
   while (offset + 4 < bufferLength) {
      if (buffer[offset + 0] == 0 &&
          buffer[offset + 1] == 0 &&
          buffer[offset + 2] == 1 &&
          H264NaluHeader::get(buffer[offset + 3]).type() == NaluType::Sps) {
         break;
      }

      ++offset;
   }

   offset += 4;
   if (offset >= bufferLength) {
      return H264Error::GenericError;
   }

   // Decode SPS
   internal::readSequenceParameterSet(nullptr,
                                      buffer + offset,
                                      bufferLength - offset,
                                      sps,
                                      nullptr);
   return H264Error::OK;
}

} // namespace internal


/**
 * Check that the stream contains sufficient data to decode an entire frame.
 */
H264Error
H264DECCheckDecunitLength(virt_ptr<void> memory,
                          virt_ptr<const uint8_t> buffer,
                          int32_t bufferLength,
                          int32_t offset,
                          virt_ptr<int32_t> outLength)
{
   if (!memory || !buffer || !outLength || bufferLength < 4 || offset < 0 ||
       bufferLength <= offset) {
      return H264Error::InvalidParameter;
   }

   auto workMemory = internal::getWorkMemory(memory);
   auto readSlice = false;
   auto readField = false;
   auto readFrameStart = false;
   auto readFrameEnd = false;
   auto start = offset;

   while (offset < bufferLength - 4) {
      // Search for NALU header
      if (buffer[offset + 0] != 0 || buffer[offset + 1] != 0 ||
          buffer[offset + 2] != 1) {
         offset++;
         continue;
      }

      auto naluType = H264NaluHeader::get(buffer[offset + 3]).type();
      auto readBuffer = buffer.get() + (offset + 4);
      auto readLength = bufferLength - (offset + 4);

      if (naluType == NaluType::Idr || naluType == NaluType::NonIdr) {
         auto bytesRead = size_t { 0 };

         StackObject<H264SliceHeader> slice;
         if (internal::readSliceHeader(workMemory->streamMemory, naluType,
                                       readBuffer, readLength,
                                       slice, &bytesRead)) {
            return H264Error::InvalidParameter;
         }

         if (readSlice && slice->first_mb_in_slice == 0) {
            // If we have read a slice and first_mb_in_slice == 0 then this
            // slice is part of the next frame.
            readFrameEnd = true;
         } else if (slice->field_pic_flag == 0) {
            // If this is not a field, then we have a frame, but we still need
            // to find the end.
            readFrameStart = true;
         } else if (readField) {
            // If this is the second field, then we have a frame, but we still
            // need to find the end.
            readFrameStart = true;
         } else {
            // First field, we need to read next field.
            readField = true;
         }

         readSlice = true;

         if (readFrameEnd) {
            break;
         }

         offset += static_cast<int32_t>(bytesRead);
      } else if (readFrameStart && naluType >= NaluType::Sps) {
         // If we are looking for the frame end, and this is a non-slice NALU
         // then we have found it!
         readFrameEnd = true;
         break;
      }

      if (naluType == NaluType::Sps) {
         auto bytesRead = size_t { 0 };

         if (internal::readSequenceParameterSet(workMemory->streamMemory,
                                                readBuffer, readLength,
                                                nullptr, &bytesRead)) {
            return H264Error::InvalidParameter;
         }

         offset += static_cast<int32_t>(bytesRead);
      } else if (naluType == NaluType::Pps) {
         auto bytesRead = size_t { 0 };

         if (internal::readPictureParameterSet(workMemory->streamMemory,
                                               readBuffer, readLength,
                                               &bytesRead)) {
            return H264Error::InvalidParameter;
         }

         offset += static_cast<int32_t>(bytesRead);
      }

      offset += 4;
   }

   if (!readFrameEnd) {
      return H264Error::GenericError;
   }

   if (offset >= bufferLength) {
      return H264Error::GenericError;
   }

   // We scan for 0 0 1, but it could be 0 0 0 1
   if (offset > 0 && buffer[offset] == 0) {
      --offset;
   }

   *outLength = offset - start;

   auto bs = BitStream {
      buffer.get() + offset,
      static_cast<size_t>(bufferLength - offset)
   };
   bs.readUE();
   if (bs.eof()) {
      return H264Error::InvalidParameter;
   }

   bs.readUE();
   if (bs.eof()) {
      return H264Error::InvalidParameter;
   }

   auto unkValue = bs.readUE();
   if (bs.eof()) {
      return H264Error::InvalidParameter;
   }

   if (unkValue >= 0x100) {
      return H264Error::GenericError;
   }

   return H264Error::OK;
}


/**
 * Check if the next NALU can be skipped without breaking decoding.
 */
H264Error
H264DECCheckSkipableFrame(virt_ptr<const uint8_t> buffer,
                          int32_t bufferLength,
                          virt_ptr<BOOL> outSkippable)
{
   if (!buffer || bufferLength < 4 || !outSkippable) {
      return H264Error::InvalidParameter;
   }

   *outSkippable = TRUE;

   for (auto i = 0; i < bufferLength - 4; ++i) {
      if (buffer[i + 0] == 0 &&
          buffer[i + 1] == 0 &&
          buffer[i + 2] == 1 &&
          H264NaluHeader::get(buffer[i + 3]).refIdc()) {
         *outSkippable = FALSE;
         return H264Error::OK;
      }
   }

   return H264Error::OK;
}


/**
 * Find the first SPS in the stream.
 */
H264Error
H264DECFindDecstartpoint(virt_ptr<const uint8_t> buffer,
                         int32_t bufferLength,
                         virt_ptr<int32_t> outOffset)
{
   if (!buffer || bufferLength < 4 || !outOffset) {
      return H264Error::InvalidParameter;
   }

   for (auto i = 0; i < bufferLength - 4; i++) {
      if (buffer[i + 0] == 0 &&
          buffer[i + 1] == 0 &&
          buffer[i + 2] == 1 &&
          H264NaluHeader::get(buffer[i + 3]).type() == NaluType::Sps) {
         *outOffset = std::max(i - 1, 0);
         return H264Error::OK;
      }
   }

   return H264Error::GenericError;
}


/**
 * Find the first "IDR point" in the stream.
 *
 * An IDR point is either:
 *  - If an SPS or PPS header is found before the IDR and there are no non-IDR
 *    inbetween the SPS/PPS and IDR then return the first of the SPS/PPS.
 *  - The first found IDR.
 */
int32_t
H264DECFindIdrpoint(virt_ptr<const uint8_t> buffer,
                    int32_t bufferLength,
                    virt_ptr<int32_t> outOffset)
{
   if (!buffer || bufferLength < 4 || !outOffset) {
      return H264Error::InvalidParameter;
   }

   auto ppsOffset = int32_t { -1 };
   auto spsOffset = int32_t { -1 };
   for (auto i = 0; i < bufferLength - 4; i++) {
      if (buffer[i + 0] != 0 || buffer[i + 1] != 0 || buffer[i + 2] != 1) {
         continue;
      }

      auto type = H264NaluHeader::get(buffer[i + 3]).type();
      if (type == NaluType::NonIdr) {
         // When we encounter a non-idr frame reset the pps / sps offset
         ppsOffset = -1;
         spsOffset = -1;
      } else if (type == NaluType::Pps) {
         ppsOffset = i - 1;
      } else if (type == NaluType::Sps) {
         spsOffset = i - 1;
      } else if (type == NaluType::Idr) {
         // Found an IDR frame!
         auto offset = std::max(i - 1, 0);

         if (ppsOffset >= 0 && spsOffset >= 0) {
            offset = std::min(ppsOffset, spsOffset);
         } else if (ppsOffset >= 0) {
            offset = ppsOffset;
         } else if (spsOffset >= 0) {
            offset = spsOffset;
         }

         return H264Error::OK;
      }
   }

   return H264Error::GenericError;
}


/**
 * Parse the H264 stream and read the width & height from the first found SPS.
 */
H264Error
H264DECGetImageSize(virt_ptr<const uint8_t> buffer,
                    int32_t bufferLength,
                    int32_t offset,
                    virt_ptr<int32_t> outWidth,
                    virt_ptr<int32_t> outHeight)
{
   // Check parameters
   if (!buffer || !outWidth || !outHeight || bufferLength < 4 || offset < 0 ||
       bufferLength <= offset) {
      return H264Error::InvalidParameter;
   }

   // Find and parse SPS
   StackObject<H264SequenceParameterSet> sps;
   auto result = internal::decodeNaluSps(buffer.get(), bufferLength, offset, sps);
   if (result != H264Error::OK) {
      return result;
   }

   // Set output
   *outWidth = sps->pic_width;
   *outHeight = sps->pic_height;

   if (!sps->frame_mbs_only_flag) {
      *outHeight = 2 * sps->pic_height;
   }

   if (!*outWidth || !*outHeight) {
      return H264Error::GenericError;
   }

   return H264Error::OK;
}

void
Library::registerStreamSymbols()
{
   RegisterFunctionExport(H264DECCheckDecunitLength);
   RegisterFunctionExport(H264DECCheckSkipableFrame);
   RegisterFunctionExport(H264DECFindDecstartpoint);
   RegisterFunctionExport(H264DECFindIdrpoint);
   RegisterFunctionExport(H264DECGetImageSize);
}

} // namespace cafe::h264
