#pragma once
#include "h264_enum.h"

#include <common/bitfield.h>
#include <libcpu/be2_struct.h>

namespace cafe::h264
{

static constexpr auto MaxPicParameterSets = 256;
static constexpr auto MaxSeqParameterSets = 32;
static constexpr auto MaxBufferedFrames = 5;
static constexpr auto SarExtended = uint8_t { 255 };

#pragma pack(push, 1)

struct H264Bitstream;
struct H264DecodeOutput;
struct H264DecodedFrameInfo;
struct H264DecodedVuiParameters;
struct H264PictureParameterSet;
struct H264SequenceParameterSet;
struct H264SliceHeader;
struct H264StreamMemory;

using H264DECFptrOutputFn = virt_func_ptr<
   void (virt_ptr<H264DecodeOutput> output)>;

struct H264Bitstream
{
   be2_virt_ptr<uint8_t> buffer;
   be2_val<uint32_t> buffer_length;
   be2_val<uint32_t> bit_position;

   //! Actually stored at +0x8F0 but that would go over into other structs
   //! memory..?
   be2_val<double> timestamp;
};
CHECK_OFFSET(H264Bitstream, 0x00, buffer);
CHECK_OFFSET(H264Bitstream, 0x04, buffer_length);
CHECK_OFFSET(H264Bitstream, 0x08, bit_position);

BITFIELD_BEG(H264NaluHeader, uint8_t)
   BITFIELD_ENTRY(7, 1, bool, forbiddenZeroBit)
   BITFIELD_ENTRY(5, 2, uint8_t, refIdc)
   BITFIELD_ENTRY(0, 5, NaluType, type)
BITFIELD_END

struct H264PictureParameterSet
{
   be2_val<uint8_t> valid;
   UNKNOWN(1);
   be2_val<uint16_t> seq_parameter_set_id;
   be2_val<uint8_t> entropy_coding_mode_flag;
   be2_val<uint8_t> pic_order_present_flag;
   be2_val<uint16_t> num_slice_groups_minus1;
   be2_val<uint16_t> slice_group_map_type;
   be2_val<uint16_t> slice_group_change_direction_flag;
   be2_val<uint16_t> slice_group_change_rate_minus1;
   be2_val<uint16_t> pic_size_in_map_units_minus1;
   be2_virt_ptr<void> ptr_to_slice_group_id;
   be2_val<uint8_t> num_ref_idx_l0_active;
   be2_val<uint8_t> num_ref_idx_l1_active;
   be2_val<uint8_t> weighted_pred_flag;
   be2_val<uint8_t> weighted_bipred_idc;
   be2_val<int16_t> pic_init_qp_minus26;
   be2_val<int16_t> pic_init_qs_minus26;
   be2_val<int16_t> chroma_qp_index_offset;
   be2_val<uint8_t> deblocking_filter_control_present_flag;
   be2_val<uint8_t> constrained_intra_pred_flag;
   be2_val<uint8_t> redundant_pic_cnt_present_flag;
   be2_val<uint8_t> transform_8x8_mode_flag;
   be2_val<uint8_t> pic_scaling_matrix_present_flag;
   be2_array<uint8_t, 8> pic_scaling_list_present_flag;
   be2_array<int8_t, 6 * 16> scalingList4x4;
   be2_array<int8_t, 2 * 64> scalingList8x8;
   UNKNOWN(1);
   be2_val<int16_t> second_chroma_qp_index_offset;
   UNKNOWN(4);
   be2_array<uint16_t, 10> run_length_minus1;
   be2_array<uint16_t, 10> top_left;
   be2_array<uint16_t, 10> bottom_right;
   UNKNOWN(2);
};
CHECK_OFFSET(H264PictureParameterSet, 0x000, valid);
CHECK_OFFSET(H264PictureParameterSet, 0x002, seq_parameter_set_id);
CHECK_OFFSET(H264PictureParameterSet, 0x004, entropy_coding_mode_flag);
CHECK_OFFSET(H264PictureParameterSet, 0x005, pic_order_present_flag);
CHECK_OFFSET(H264PictureParameterSet, 0x006, num_slice_groups_minus1);
CHECK_OFFSET(H264PictureParameterSet, 0x008, slice_group_map_type);
CHECK_OFFSET(H264PictureParameterSet, 0x00A, slice_group_change_direction_flag);
CHECK_OFFSET(H264PictureParameterSet, 0x00C, slice_group_change_rate_minus1);
CHECK_OFFSET(H264PictureParameterSet, 0x00E, pic_size_in_map_units_minus1);
CHECK_OFFSET(H264PictureParameterSet, 0x010, ptr_to_slice_group_id);
CHECK_OFFSET(H264PictureParameterSet, 0x014, num_ref_idx_l0_active);
CHECK_OFFSET(H264PictureParameterSet, 0x015, num_ref_idx_l1_active);
CHECK_OFFSET(H264PictureParameterSet, 0x016, weighted_pred_flag);
CHECK_OFFSET(H264PictureParameterSet, 0x017, weighted_bipred_idc);
CHECK_OFFSET(H264PictureParameterSet, 0x018, pic_init_qp_minus26);
CHECK_OFFSET(H264PictureParameterSet, 0x01A, pic_init_qs_minus26);
CHECK_OFFSET(H264PictureParameterSet, 0x01C, chroma_qp_index_offset);
CHECK_OFFSET(H264PictureParameterSet, 0x01E, deblocking_filter_control_present_flag);
CHECK_OFFSET(H264PictureParameterSet, 0x01F, constrained_intra_pred_flag);
CHECK_OFFSET(H264PictureParameterSet, 0x020, redundant_pic_cnt_present_flag);
CHECK_OFFSET(H264PictureParameterSet, 0x021, transform_8x8_mode_flag);
CHECK_OFFSET(H264PictureParameterSet, 0x022, pic_scaling_matrix_present_flag);
CHECK_OFFSET(H264PictureParameterSet, 0x023, pic_scaling_list_present_flag);
CHECK_OFFSET(H264PictureParameterSet, 0x02B, scalingList4x4);
CHECK_OFFSET(H264PictureParameterSet, 0x08B, scalingList8x8);
CHECK_OFFSET(H264PictureParameterSet, 0x10C, second_chroma_qp_index_offset);
CHECK_OFFSET(H264PictureParameterSet, 0x112, run_length_minus1);
CHECK_OFFSET(H264PictureParameterSet, 0x126, top_left);
CHECK_OFFSET(H264PictureParameterSet, 0x13A, bottom_right);
CHECK_SIZE(H264PictureParameterSet, 0x150);

struct H264SequenceParameterSet
{
   be2_val<uint8_t> valid;
   UNKNOWN(0x1);
   be2_val<uint16_t> profile_idc;
   be2_val<uint16_t> level_idc;
   be2_val<uint8_t> constraint_set;
   be2_val<uint8_t> seq_parameter_set_id;
   be2_val<uint8_t> chroma_format_idc;
   be2_val<uint8_t> residual_colour_transform_flag;
   be2_val<uint8_t> bit_depth_luma_minus8;
   be2_val<uint8_t> bit_depth_chroma_minus8;
   be2_val<uint8_t> qpprime_y_zero_transform_bypass_flag;
   be2_val<uint8_t> seq_scaling_matrix_present_flag;
   be2_array<int8_t, 6 * 16> scalingList4x4;
   be2_array<int8_t, 2 * 64> scalingList8x8;
   be2_val<uint16_t> log2_max_frame_num_minus4;
   be2_val<uint16_t> pic_order_cnt_type;
   be2_val<uint16_t> log2_max_pic_order_cnt_lsb_minus4;
   be2_val<uint16_t> delta_pic_order_always_zero_flag;
   PADDING(0x2);
   be2_val<int32_t> offset_for_non_ref_pic;
   be2_val<int32_t> offset_for_top_to_bottom_field;
   be2_val<uint16_t> num_ref_frames_in_pic_order_cnt_cycle;
   PADDING(0x2);
   be2_array<int32_t, 256> offset_for_ref_frame;
   be2_val<uint16_t> num_ref_frames;
   be2_val<uint8_t> gaps_in_frame_num_value_allowed_flag;
   PADDING(0x1);
   be2_val<uint16_t> pic_width_in_mbs;
   be2_val<uint16_t> pic_height_in_map_units;
   be2_val<uint8_t> frame_mbs_only_flag;
   be2_val<uint8_t> mb_adaptive_frame_field_flag;
   be2_val<uint8_t> direct_8x8_inference_flag;
   be2_val<uint8_t> frame_cropping_flag;
   be2_val<uint16_t> frame_crop_left_offset;
   be2_val<uint16_t> frame_crop_right_offset;
   be2_val<uint16_t> frame_crop_top_offset;
   be2_val<uint16_t> frame_crop_bottom_offset;
   be2_val<uint8_t> vui_parameters_present_flag;
   be2_val<uint8_t> vui_aspect_ratio_info_present_flag;
   be2_val<uint8_t> vui_aspect_ratio_idc;
   PADDING(0x1);
   be2_val<uint16_t> vui_sar_width;
   be2_val<uint16_t> vui_sar_height;
   be2_val<uint8_t> vui_overscan_info_present_flag;
   be2_val<uint8_t> vui_overscan_appropriate_flag;
   be2_val<uint8_t> vui_video_signal_type_present_flag;
   be2_val<uint8_t> vui_video_format;
   be2_val<uint8_t> vui_video_full_range_flag;
   be2_val<uint8_t> vui_colour_description_present_flag;
   be2_val<uint8_t> vui_colour_primaries;
   be2_val<uint8_t> vui_transfer_characteristics;
   be2_val<uint8_t> vui_matrix_coefficients;
   be2_val<uint8_t> vui_chroma_loc_info_present_flag;
   be2_val<uint8_t> vui_chroma_sample_loc_type_top_field;
   be2_val<uint8_t> vui_chroma_sample_loc_type_bottom_field;
   be2_val<uint8_t> vui_timing_info_present_flag;
   PADDING(0x3);
   be2_val<uint32_t> vui_num_units_in_tick;
   be2_val<uint32_t> vui_time_scale;
   be2_val<uint8_t> vui_fixed_frame_rate_flag;
   be2_val<uint8_t> vui_nal_hrd_parameters_present_flag;
   be2_val<uint8_t> vui_vcl_hrd_parameters_present_flag;
   PADDING(0x1);
   be2_val<uint16_t> hrd_cpb_cnt_minus1;
   be2_val<uint16_t> hrd_bit_rate_scale;
   be2_val<uint16_t> hrd_cpb_size_scale;
   be2_array<uint16_t, 100> hrd_bit_rate_value_minus1;
   be2_array<uint16_t, 100> hrd_cpb_size_value_minus1;
   be2_array<uint16_t, 100> hrd_cbr_flag;
   be2_val<uint16_t> hrd_initial_cpb_removal_delay_length_minus1;
   be2_val<uint16_t> hrd_cpb_removal_delay_length_minus1;
   be2_val<uint16_t> hrd_dpb_output_delay_length_minus1;
   be2_val<uint16_t> hrd_time_offset_length;
   be2_val<uint8_t> vui_low_delay_hrd_flag;
   be2_val<uint8_t> vui_pic_struct_present_flag;
   be2_val<uint8_t> vui_bitstream_restriction_flag;
   be2_val<uint8_t> vui_motion_vectors_over_pic_boundaries_flag;
   be2_val<uint16_t> vui_max_bytes_per_pic_denom;
   be2_val<uint16_t> vui_max_bits_per_mb_denom;
   be2_val<uint16_t> vui_log2_max_mv_length_horizontal;
   be2_val<uint16_t> vui_log2_max_mv_length_vertical;
   be2_val<uint16_t> vui_num_reorder_frames;
   be2_val<uint16_t> unk0x7B0;
   be2_val<uint16_t> vui_max_dec_frame_buffering;
   be2_val<uint32_t> max_pic_order_cnt_lsb;
   be2_val<uint16_t> unk0x7B8;
   UNKNOWN(0x2);
   be2_val<uint16_t> pic_size_in_mbs;
   UNKNOWN(0x2);
   be2_val<uint32_t> max_frame_num;
   be2_val<uint8_t> log2_max_frame_num;
   be2_val<uint8_t> unk0x7C5;
   be2_val<uint16_t> unk0x7C6;
   be2_val<uint16_t> pic_width;
   be2_val<uint16_t> pic_height;
   be2_val<uint16_t> unk0x7CC;
   UNKNOWN(0x95E - 0x7CE);
   be2_val<uint8_t> unk0x95E;
   be2_val<uint8_t> unk0x95F;
   be2_val<uint8_t> unk0x960;
   UNKNOWN(0x978 - 0x961);
};
CHECK_OFFSET(H264SequenceParameterSet, 0x000, valid);
CHECK_OFFSET(H264SequenceParameterSet, 0x002, profile_idc);
CHECK_OFFSET(H264SequenceParameterSet, 0x004, level_idc);
CHECK_OFFSET(H264SequenceParameterSet, 0x006, constraint_set);
CHECK_OFFSET(H264SequenceParameterSet, 0x007, seq_parameter_set_id);
CHECK_OFFSET(H264SequenceParameterSet, 0x008, chroma_format_idc);
CHECK_OFFSET(H264SequenceParameterSet, 0x00A, bit_depth_luma_minus8);
CHECK_OFFSET(H264SequenceParameterSet, 0x00B, bit_depth_chroma_minus8);
CHECK_OFFSET(H264SequenceParameterSet, 0x00C, qpprime_y_zero_transform_bypass_flag);
CHECK_OFFSET(H264SequenceParameterSet, 0x00D, seq_scaling_matrix_present_flag);
CHECK_OFFSET(H264SequenceParameterSet, 0x00E, scalingList4x4);
CHECK_OFFSET(H264SequenceParameterSet, 0x06E, scalingList8x8);
CHECK_OFFSET(H264SequenceParameterSet, 0x0EE, log2_max_frame_num_minus4);
CHECK_OFFSET(H264SequenceParameterSet, 0x0F0, pic_order_cnt_type);
CHECK_OFFSET(H264SequenceParameterSet, 0x0F2, log2_max_pic_order_cnt_lsb_minus4);
CHECK_OFFSET(H264SequenceParameterSet, 0x0F4, delta_pic_order_always_zero_flag);
CHECK_OFFSET(H264SequenceParameterSet, 0x0F8, offset_for_non_ref_pic);
CHECK_OFFSET(H264SequenceParameterSet, 0x0FC, offset_for_top_to_bottom_field);
CHECK_OFFSET(H264SequenceParameterSet, 0x100, num_ref_frames_in_pic_order_cnt_cycle);
CHECK_OFFSET(H264SequenceParameterSet, 0x104, offset_for_ref_frame);
CHECK_OFFSET(H264SequenceParameterSet, 0x504, num_ref_frames);
CHECK_OFFSET(H264SequenceParameterSet, 0x506, gaps_in_frame_num_value_allowed_flag);
CHECK_OFFSET(H264SequenceParameterSet, 0x508, pic_width_in_mbs);
CHECK_OFFSET(H264SequenceParameterSet, 0x50A, pic_height_in_map_units);
CHECK_OFFSET(H264SequenceParameterSet, 0x50C, frame_mbs_only_flag);
CHECK_OFFSET(H264SequenceParameterSet, 0x50D, mb_adaptive_frame_field_flag);
CHECK_OFFSET(H264SequenceParameterSet, 0x50E, direct_8x8_inference_flag);
CHECK_OFFSET(H264SequenceParameterSet, 0x50F, frame_cropping_flag);
CHECK_OFFSET(H264SequenceParameterSet, 0x510, frame_crop_left_offset);
CHECK_OFFSET(H264SequenceParameterSet, 0x512, frame_crop_right_offset);
CHECK_OFFSET(H264SequenceParameterSet, 0x514, frame_crop_top_offset);
CHECK_OFFSET(H264SequenceParameterSet, 0x516, frame_crop_bottom_offset);
CHECK_OFFSET(H264SequenceParameterSet, 0x518, vui_parameters_present_flag);
CHECK_OFFSET(H264SequenceParameterSet, 0x519, vui_aspect_ratio_info_present_flag);
CHECK_OFFSET(H264SequenceParameterSet, 0x51A, vui_aspect_ratio_idc);
CHECK_OFFSET(H264SequenceParameterSet, 0x51C, vui_sar_width);
CHECK_OFFSET(H264SequenceParameterSet, 0x51E, vui_sar_height);
CHECK_OFFSET(H264SequenceParameterSet, 0x520, vui_overscan_info_present_flag);
CHECK_OFFSET(H264SequenceParameterSet, 0x521, vui_overscan_appropriate_flag);
CHECK_OFFSET(H264SequenceParameterSet, 0x522, vui_video_signal_type_present_flag);
CHECK_OFFSET(H264SequenceParameterSet, 0x523, vui_video_format);
CHECK_OFFSET(H264SequenceParameterSet, 0x524, vui_video_full_range_flag);
CHECK_OFFSET(H264SequenceParameterSet, 0x525, vui_colour_description_present_flag);
CHECK_OFFSET(H264SequenceParameterSet, 0x526, vui_colour_primaries);
CHECK_OFFSET(H264SequenceParameterSet, 0x527, vui_transfer_characteristics);
CHECK_OFFSET(H264SequenceParameterSet, 0x528, vui_matrix_coefficients);
CHECK_OFFSET(H264SequenceParameterSet, 0x529, vui_chroma_loc_info_present_flag);
CHECK_OFFSET(H264SequenceParameterSet, 0x52A, vui_chroma_sample_loc_type_top_field);
CHECK_OFFSET(H264SequenceParameterSet, 0x52B, vui_chroma_sample_loc_type_bottom_field);
CHECK_OFFSET(H264SequenceParameterSet, 0x52C, vui_timing_info_present_flag);
CHECK_OFFSET(H264SequenceParameterSet, 0x530, vui_num_units_in_tick);
CHECK_OFFSET(H264SequenceParameterSet, 0x534, vui_time_scale);
CHECK_OFFSET(H264SequenceParameterSet, 0x538, vui_fixed_frame_rate_flag);
CHECK_OFFSET(H264SequenceParameterSet, 0x539, vui_nal_hrd_parameters_present_flag);
CHECK_OFFSET(H264SequenceParameterSet, 0x53A, vui_vcl_hrd_parameters_present_flag);
CHECK_OFFSET(H264SequenceParameterSet, 0x53C, hrd_cpb_cnt_minus1);
CHECK_OFFSET(H264SequenceParameterSet, 0x53E, hrd_bit_rate_scale);
CHECK_OFFSET(H264SequenceParameterSet, 0x540, hrd_cpb_size_scale);
CHECK_OFFSET(H264SequenceParameterSet, 0x542, hrd_bit_rate_value_minus1);
CHECK_OFFSET(H264SequenceParameterSet, 0x60A, hrd_cpb_size_value_minus1);
CHECK_OFFSET(H264SequenceParameterSet, 0x6D2, hrd_cbr_flag);
CHECK_OFFSET(H264SequenceParameterSet, 0x79A, hrd_initial_cpb_removal_delay_length_minus1);
CHECK_OFFSET(H264SequenceParameterSet, 0x79C, hrd_cpb_removal_delay_length_minus1);
CHECK_OFFSET(H264SequenceParameterSet, 0x79E, hrd_dpb_output_delay_length_minus1);
CHECK_OFFSET(H264SequenceParameterSet, 0x7A0, hrd_time_offset_length);
CHECK_OFFSET(H264SequenceParameterSet, 0x7A2, vui_low_delay_hrd_flag);
CHECK_OFFSET(H264SequenceParameterSet, 0x7A3, vui_pic_struct_present_flag);
CHECK_OFFSET(H264SequenceParameterSet, 0x7A4, vui_bitstream_restriction_flag);
CHECK_OFFSET(H264SequenceParameterSet, 0x7A5, vui_motion_vectors_over_pic_boundaries_flag);
CHECK_OFFSET(H264SequenceParameterSet, 0x7A6, vui_max_bytes_per_pic_denom);
CHECK_OFFSET(H264SequenceParameterSet, 0x7A8, vui_max_bits_per_mb_denom);
CHECK_OFFSET(H264SequenceParameterSet, 0x7AA, vui_log2_max_mv_length_horizontal);
CHECK_OFFSET(H264SequenceParameterSet, 0x7AC, vui_log2_max_mv_length_vertical);
CHECK_OFFSET(H264SequenceParameterSet, 0x7AE, vui_num_reorder_frames);
CHECK_OFFSET(H264SequenceParameterSet, 0x7B2, vui_max_dec_frame_buffering);
CHECK_OFFSET(H264SequenceParameterSet, 0x7B4, max_pic_order_cnt_lsb);
CHECK_OFFSET(H264SequenceParameterSet, 0x7BC, pic_size_in_mbs);
CHECK_OFFSET(H264SequenceParameterSet, 0x7C0, max_frame_num);
CHECK_OFFSET(H264SequenceParameterSet, 0x7C4, log2_max_frame_num);
CHECK_OFFSET(H264SequenceParameterSet, 0x7C8, pic_width);
CHECK_OFFSET(H264SequenceParameterSet, 0x7CA, pic_height);
CHECK_SIZE(H264SequenceParameterSet, 0x978);

struct H264SliceHeader
{
   UNKNOWN(2);
   be2_val<uint8_t> field_02;
   UNKNOWN(1);
   be2_val<uint16_t> pic_parameter_set_id;
   be2_val<uint16_t> frame_num;
   be2_val<uint32_t> idr_pic_id;
   be2_val<uint32_t> pic_order_cnt_lsb;
   be2_val<int32_t> delta_pic_order_cnt_bottom;
   be2_val<uint32_t> field_14;
   be2_val<uint32_t> field_18;
   be2_val<uint8_t> field_1C;
   be2_val<uint8_t> field_1D;
   be2_val<uint8_t> field_pic_flag;
   be2_val<uint8_t> bottom_field_flag;
   be2_val<uint16_t> field_20;
   be2_val<uint16_t> first_mb_in_slice;
   be2_val<uint8_t> slice_type;
   be2_val<uint8_t> field_25;
   be2_val<uint8_t> field_26;
   be2_val<uint8_t> field_27;
   be2_array<int32_t, 2> delta_pic_order_cnt;
   be2_val<uint16_t> redundant_pic_cnt;
   be2_val<uint8_t> direct_spatial_mv_pred_flag;
   be2_val<uint8_t> num_ref_idx_active_override_flag;
   be2_val<uint8_t> num_ref_idx_l0_active;
   be2_val<uint8_t> num_ref_idx_l1_active;
};
CHECK_OFFSET(H264SliceHeader, 0x02, field_02);
CHECK_OFFSET(H264SliceHeader, 0x04, pic_parameter_set_id);
CHECK_OFFSET(H264SliceHeader, 0x06, frame_num);
CHECK_OFFSET(H264SliceHeader, 0x08, idr_pic_id);
CHECK_OFFSET(H264SliceHeader, 0x0C, pic_order_cnt_lsb);
CHECK_OFFSET(H264SliceHeader, 0x10, delta_pic_order_cnt_bottom);
CHECK_OFFSET(H264SliceHeader, 0x14, field_14);
CHECK_OFFSET(H264SliceHeader, 0x18, field_18);
CHECK_OFFSET(H264SliceHeader, 0x1C, field_1C);
CHECK_OFFSET(H264SliceHeader, 0x1D, field_1D);
CHECK_OFFSET(H264SliceHeader, 0x1E, field_pic_flag);
CHECK_OFFSET(H264SliceHeader, 0x1F, bottom_field_flag);
CHECK_OFFSET(H264SliceHeader, 0x20, field_20);
CHECK_OFFSET(H264SliceHeader, 0x22, first_mb_in_slice);
CHECK_OFFSET(H264SliceHeader, 0x24, slice_type);
CHECK_OFFSET(H264SliceHeader, 0x25, field_25);
CHECK_OFFSET(H264SliceHeader, 0x26, field_26);
CHECK_OFFSET(H264SliceHeader, 0x27, field_27);
CHECK_OFFSET(H264SliceHeader, 0x28, delta_pic_order_cnt);
CHECK_OFFSET(H264SliceHeader, 0x30, redundant_pic_cnt);
CHECK_OFFSET(H264SliceHeader, 0x32, direct_spatial_mv_pred_flag);
CHECK_OFFSET(H264SliceHeader, 0x33, num_ref_idx_active_override_flag);
CHECK_OFFSET(H264SliceHeader, 0x34, num_ref_idx_l0_active);
CHECK_OFFSET(H264SliceHeader, 0x35, num_ref_idx_l1_active);
CHECK_SIZE(H264SliceHeader, 0x36);

struct H264DecodedVuiParameters
{
   be2_val<uint8_t> aspect_ratio_info_present_flag;
   be2_val<uint8_t> aspect_ratio_idc;
   be2_val<int16_t> sar_width;
   be2_val<int16_t> sar_height;
   be2_val<uint8_t> overscan_info_present_flag;
   be2_val<uint8_t> overscan_appropriate_flag;
   be2_val<uint8_t> video_signal_type_present_flag;
   be2_val<uint8_t> video_format;
   be2_val<uint8_t> video_full_range_flag;
   be2_val<uint8_t> colour_description_present_flag;
   be2_val<uint8_t> colour_primaries;
   be2_val<uint8_t> transfer_characteristics;
   be2_val<uint8_t> matrix_coefficients;
   be2_val<uint8_t> chroma_loc_info_present_flag;
   be2_val<uint8_t> chroma_sample_loc_type_top_field;
   be2_val<uint8_t> chroma_sample_loc_type_bottom_field;
   be2_val<uint8_t> timing_info_present_flag;
   PADDING(1);
   be2_val<uint32_t> num_units_in_tick;
   be2_val<uint32_t> time_scale;
   be2_val<uint8_t> fixed_frame_rate_flag;
   be2_val<uint8_t> nal_hrd_parameters_present_flag;
   be2_val<uint8_t> vcl_hrd_parameters_present_flag;
   be2_val<uint8_t> low_delay_hrd_flag;
   be2_val<uint8_t> pic_struct_present_flag;
   be2_val<uint8_t> bitstream_restriction_flag;
   be2_val<uint8_t> motion_vectors_over_pic_boundaries_flag;
   PADDING(1);
   be2_val<int16_t> max_bytes_per_pic_denom;
   be2_val<int16_t> max_bits_per_mb_denom;
   be2_val<int16_t> log2_max_mv_length_horizontal;
   be2_val<int16_t> log2_max_mv_length_vertical;
   be2_val<int16_t> num_reorder_frames;
   be2_val<int16_t> max_dec_frame_buffering;
};
CHECK_OFFSET(H264DecodedVuiParameters, 0x00, aspect_ratio_info_present_flag);
CHECK_OFFSET(H264DecodedVuiParameters, 0x01, aspect_ratio_idc);
CHECK_OFFSET(H264DecodedVuiParameters, 0x02, sar_width);
CHECK_OFFSET(H264DecodedVuiParameters, 0x04, sar_height);
CHECK_OFFSET(H264DecodedVuiParameters, 0x06, overscan_info_present_flag);
CHECK_OFFSET(H264DecodedVuiParameters, 0x07, overscan_appropriate_flag);
CHECK_OFFSET(H264DecodedVuiParameters, 0x08, video_signal_type_present_flag);
CHECK_OFFSET(H264DecodedVuiParameters, 0x09, video_format);
CHECK_OFFSET(H264DecodedVuiParameters, 0x0A, video_full_range_flag);
CHECK_OFFSET(H264DecodedVuiParameters, 0x0B, colour_description_present_flag);
CHECK_OFFSET(H264DecodedVuiParameters, 0x0C, colour_primaries);
CHECK_OFFSET(H264DecodedVuiParameters, 0x0D, transfer_characteristics);
CHECK_OFFSET(H264DecodedVuiParameters, 0x0E, matrix_coefficients);
CHECK_OFFSET(H264DecodedVuiParameters, 0x0F, chroma_loc_info_present_flag);
CHECK_OFFSET(H264DecodedVuiParameters, 0x10, chroma_sample_loc_type_top_field);
CHECK_OFFSET(H264DecodedVuiParameters, 0x11, chroma_sample_loc_type_bottom_field);
CHECK_OFFSET(H264DecodedVuiParameters, 0x12, timing_info_present_flag);
CHECK_OFFSET(H264DecodedVuiParameters, 0x14, num_units_in_tick);
CHECK_OFFSET(H264DecodedVuiParameters, 0x18, time_scale);
CHECK_OFFSET(H264DecodedVuiParameters, 0x1C, fixed_frame_rate_flag);
CHECK_OFFSET(H264DecodedVuiParameters, 0x1D, nal_hrd_parameters_present_flag);
CHECK_OFFSET(H264DecodedVuiParameters, 0x1E, vcl_hrd_parameters_present_flag);
CHECK_OFFSET(H264DecodedVuiParameters, 0x1F, low_delay_hrd_flag);
CHECK_OFFSET(H264DecodedVuiParameters, 0x20, pic_struct_present_flag);
CHECK_OFFSET(H264DecodedVuiParameters, 0x21, bitstream_restriction_flag);
CHECK_OFFSET(H264DecodedVuiParameters, 0x22, motion_vectors_over_pic_boundaries_flag);
CHECK_OFFSET(H264DecodedVuiParameters, 0x24, max_bytes_per_pic_denom);
CHECK_OFFSET(H264DecodedVuiParameters, 0x26, max_bits_per_mb_denom);
CHECK_OFFSET(H264DecodedVuiParameters, 0x28, log2_max_mv_length_horizontal);
CHECK_OFFSET(H264DecodedVuiParameters, 0x2A, log2_max_mv_length_vertical);
CHECK_OFFSET(H264DecodedVuiParameters, 0x2C, num_reorder_frames);
CHECK_OFFSET(H264DecodedVuiParameters, 0x2E, max_dec_frame_buffering);
CHECK_SIZE(H264DecodedVuiParameters, 0x30);

struct H264DecodedFrameInfo
{
   // Timestamp is actually stored in dpb but we are lazy and don't have dpb
   be2_val<double> timestamp;

   UNKNOWN(0x30 - 8);

   be2_virt_ptr<void> buffer;
   be2_val<uint8_t> pan_scan_enable_flag;
   PADDING(1);
   be2_val<uint16_t> left_pan_scan;
   be2_val<uint16_t> right_pan_scan;
   be2_val<uint16_t> top_pan_scan;
   be2_val<uint16_t> bottom_pan_scan;
   UNKNOWN(1);
   be2_val<uint8_t> vui_parameters_present_flag;
   be2_struct<H264DecodedVuiParameters> vui_parameters;
};
CHECK_OFFSET(H264DecodedFrameInfo, 0x30, buffer);
CHECK_OFFSET(H264DecodedFrameInfo, 0x34, pan_scan_enable_flag);
CHECK_OFFSET(H264DecodedFrameInfo, 0x36, left_pan_scan);
CHECK_OFFSET(H264DecodedFrameInfo, 0x38, right_pan_scan);
CHECK_OFFSET(H264DecodedFrameInfo, 0x3A, top_pan_scan);
CHECK_OFFSET(H264DecodedFrameInfo, 0x3C, bottom_pan_scan);
CHECK_OFFSET(H264DecodedFrameInfo, 0x3F, vui_parameters_present_flag);
CHECK_OFFSET(H264DecodedFrameInfo, 0x40, vui_parameters);
CHECK_SIZE(H264DecodedFrameInfo, 0x70);

struct H264StreamMemory
{
   be2_virt_ptr<void> workMemoryEnd;
   be2_virt_ptr<void> unkMemory;
   UNKNOWN(0x7C4 - 0x8);
   be2_array<H264DecodedFrameInfo, 6> decodedFrameInfos;
   be2_struct<H264SequenceParameterSet> currentSps;
   be2_virt_ptr<H264SequenceParameterSet> currentSpsPtr;
   be2_struct<H264PictureParameterSet> currentPps;
   UNKNOWN(0x107BC - 0x1530);
   be2_val<H264DECFptrOutputFn> paramFramePointerOutput;
   be2_array<H264SequenceParameterSet, MaxSeqParameterSets> spsTable;
   be2_array<H264PictureParameterSet, MaxPicParameterSets> ppsTable;
   UNKNOWN(0x38AD8 - 0x386C0);
   be2_val<uint32_t> param_0x20000030;
   be2_val<uint32_t> param_0x20000040;
   UNKNOWN(0x8);
   be2_val<uint32_t> frameBufferIndex;
   be2_virt_ptr<void> frameBufferPtr;
   be2_virt_ptr<void> paramUserMemory;
   be2_val<uint8_t> paramOutputPerFrame;
   UNKNOWN(0x10E6B4 - (0x38AF4 + 1));
};
CHECK_OFFSET(H264StreamMemory, 0x0, workMemoryEnd);
CHECK_OFFSET(H264StreamMemory, 0x4, unkMemory);
CHECK_OFFSET(H264StreamMemory, 0x7C4, decodedFrameInfos);
CHECK_OFFSET(H264StreamMemory, 0xA64, currentSps);
CHECK_OFFSET(H264StreamMemory, 0x13DC, currentSpsPtr);
CHECK_OFFSET(H264StreamMemory, 0x13E0, currentPps);
CHECK_OFFSET(H264StreamMemory, 0x107BC, paramFramePointerOutput);
CHECK_OFFSET(H264StreamMemory, 0x107C0, spsTable);
CHECK_OFFSET(H264StreamMemory, 0x236C0, ppsTable);
CHECK_OFFSET(H264StreamMemory, 0x38AD8, param_0x20000030);
CHECK_OFFSET(H264StreamMemory, 0x38ADC, param_0x20000040);
CHECK_OFFSET(H264StreamMemory, 0x38AE8, frameBufferIndex);
CHECK_OFFSET(H264StreamMemory, 0x38AEC, frameBufferPtr);
CHECK_OFFSET(H264StreamMemory, 0x38AF0, paramUserMemory);
CHECK_OFFSET(H264StreamMemory, 0x38AF4, paramOutputPerFrame);
CHECK_SIZE(H264StreamMemory, 0x10E6B4);

struct H264DecodeResult
{
   be2_val<int32_t> status;
   PADDING(4);
   be2_val<double> timestamp;
   be2_val<int32_t> width;
   be2_val<int32_t> height;
   be2_val<int32_t> nextLine;
   be2_val<uint8_t> cropEnableFlag;
   PADDING(3);
   be2_val<int32_t> cropTop;
   be2_val<int32_t> cropBottom;
   be2_val<int32_t> cropLeft;
   be2_val<int32_t> cropRight;
   be2_val<uint8_t> panScanEnableFlag;
   PADDING(3);
   be2_val<int32_t> panScanTop;
   be2_val<int32_t> panScanBottom;
   be2_val<int32_t> panScanLeft;
   be2_val<int32_t> panScanRight;
   be2_virt_ptr<void> framebuffer;
   be2_val<uint8_t> vui_parameters_present_flag;
   PADDING(3);
   be2_virt_ptr<H264DecodedVuiParameters> vui_parameters;
   UNKNOWN(40);
};
CHECK_OFFSET(H264DecodeResult, 0x00, status);
CHECK_OFFSET(H264DecodeResult, 0x08, timestamp);
CHECK_OFFSET(H264DecodeResult, 0x10, width);
CHECK_OFFSET(H264DecodeResult, 0x14, height);
CHECK_OFFSET(H264DecodeResult, 0x18, nextLine);
CHECK_OFFSET(H264DecodeResult, 0x1C, cropEnableFlag);
CHECK_OFFSET(H264DecodeResult, 0x20, cropTop);
CHECK_OFFSET(H264DecodeResult, 0x24, cropBottom);
CHECK_OFFSET(H264DecodeResult, 0x28, cropLeft);
CHECK_OFFSET(H264DecodeResult, 0x2C, cropRight);
CHECK_OFFSET(H264DecodeResult, 0x30, panScanEnableFlag);
CHECK_OFFSET(H264DecodeResult, 0x34, panScanTop);
CHECK_OFFSET(H264DecodeResult, 0x38, panScanBottom);
CHECK_OFFSET(H264DecodeResult, 0x3C, panScanLeft);
CHECK_OFFSET(H264DecodeResult, 0x40, panScanRight);
CHECK_OFFSET(H264DecodeResult, 0x44, framebuffer);
CHECK_OFFSET(H264DecodeResult, 0x48, vui_parameters_present_flag);
CHECK_OFFSET(H264DecodeResult, 0x4C, vui_parameters);
CHECK_SIZE(H264DecodeResult, 0x78);

struct H264DecodeOutput
{
   //! Number of frames output
   be2_val<int32_t> frameCount;

   //! Frames
   be2_virt_ptr<virt_ptr<H264DecodeResult>> decodeResults;

   //! User memory pointer passed into SetParam
   be2_virt_ptr<void> userMemory;
};
CHECK_OFFSET(H264DecodeOutput, 0x00, frameCount);
CHECK_OFFSET(H264DecodeOutput, 0x04, decodeResults);
CHECK_OFFSET(H264DecodeOutput, 0x08, userMemory);
CHECK_SIZE(H264DecodeOutput, 0x0C);

#pragma pack(pop)

H264Error
H264DECCheckDecunitLength(virt_ptr<void> memory,
                          virt_ptr<const uint8_t> buffer,
                          int32_t bufferLength,
                          int32_t offset,
                          virt_ptr<int32_t> outLength);

H264Error
H264DECCheckSkipableFrame(virt_ptr<const uint8_t> buffer,
                          int32_t bufferLength,
                          virt_ptr<BOOL> outSkippable);

H264Error
H264DECFindDecstartpoint(virt_ptr<const uint8_t> buffer,
                         int32_t bufferLength,
                         virt_ptr<int32_t> outOffset);

int32_t
H264DECFindIdrpoint(virt_ptr<const uint8_t> buffer,
                    int32_t bufferLength,
                    virt_ptr<int32_t> outOffset);

H264Error
H264DECGetImageSize(virt_ptr<const uint8_t> buffer,
                    int32_t bufferLength,
                    int32_t offset,
                    virt_ptr<int32_t> outWidth,
                    virt_ptr<int32_t> outHeight);

namespace internal
{

H264Error
decodeNaluSps(const uint8_t *buffer,
              int bufferLength,
              int offset,
              virt_ptr<H264SequenceParameterSet> sps);

} // namespace internal

} // namespace cafe::h264
