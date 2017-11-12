#pragma once
#include "Common.h"

#define IN
#define OUT

typedef struct Struct_Thread_Incomplete_EpilogueT_Information Thread_Incomplete_EpilogueT_Information;
typedef struct Struct_Thread_Block_EpilogueT_Information Thread_Block_EpilogueT_Information;

struct Struct_Thread_Incomplete_EpilogueT_Information
{
	IN const Block* block;
	IN const u8* image_bytes;
	IN const Image_Information* image_information;
	IN s32* domain_transforms;
	IN r32** rf_table;
	IN s32 iteration;
	OUT r32* last_epilogueT_contribution;
	OUT r32* incomplete_epilogueT;
};

struct Struct_Thread_Block_EpilogueT_Information
{
	IN const Block* block;
	IN const u8* image_bytes;
	IN const Image_Information* image_information;
	IN s32* domain_transforms;
	IN r32** rf_table;
	IN s32 iteration;
	IN r32* last_epiloguesT;
	OUT u8* image_result;
};

extern void calculate_incomplete_epiloguesT(const u8* image_bytes,
	Image_Information* image_information,
	r32** epiloguesT,
	r32** last_epilogueT_contributions,
	s32 parallelism_level_x,
	s32 parallelism_level_y,
	const Block* image_blocks,
	s32* vertical_domain_transforms,
	r32** rf_table,
	s32 iteration,
	s32 number_of_threads);

extern void calculate_blocks_from_epiloguesT(const u8* image_bytes,
	Image_Information* image_information,
	r32** epiloguesT,
	s32 parallelism_level_x,
	s32 parallelism_level_y,
	const Block* image_blocks,
	s32* vertical_domain_transforms,
	r32** rf_table,
	s32 iteration,
	s32 number_of_threads,
	u8* image_result);

extern void calculate_complete_epiloguesT(Image_Information* image_information,
	r32** complete_epiloguesT,
	r32** incomplete_epiloguesT,
	r32** last_epilogueT_contributions,
	Block* image_blocks,
	s32 parallelism_level_x,
	s32 parallelism_level_y);