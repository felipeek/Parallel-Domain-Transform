#pragma once
#include "Common.h"

extern s32 parallel_colorization(const r32* image_bytes,
	const r32* scribbles_bytes,
	const r32* mask_bytes,
	s32 image_width,
	s32 image_height,
	s32 image_channels,
	r32 spatial_factor,
	r32 range_factor,
	s32 num_iterations,
	s32 number_of_threads,
	s32 parallelism_level_x,
	s32 parallelism_level_y,
	r32* image_result);