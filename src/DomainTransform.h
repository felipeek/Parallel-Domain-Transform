#pragma once
#include "Common.h"

extern s32 domain_transform(const r32* image_bytes, s32 image_width, s32 image_height, s32 image_channels, r32 spatial_factor, r32 range_factor,
	s32 num_iterations, r32* image_result);