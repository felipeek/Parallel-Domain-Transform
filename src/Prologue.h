#pragma once
#include "Common.h"

#define IN
#define OUT

typedef struct Struct_Thread_Incomplete_Prologue_Information Thread_Incomplete_Prologue_Information;
typedef struct Struct_Thread_Block_Prologue_Information Thread_Block_Prologue_Information;

struct Struct_Thread_Incomplete_Prologue_Information
{
	IN const Block* block;
	IN const u8* image_bytes;
	IN const Image_Information* image_information;
	IN s32* domain_transforms;
	IN r32** rf_table;
	IN s32 iteration;
	OUT r32* last_prologue_contribution;
	OUT r32* incomplete_prologue;
};

struct Struct_Thread_Block_Prologue_Information
{
	IN const Block* block;
	IN const u8* image_bytes;
	IN const Image_Information* image_information;
	IN s32* domain_transforms;
	IN r32** rf_table;
	IN s32 iteration;
	IN r32* last_prologues;
	OUT u8* image_result;
};

extern void calculate_incomplete_prologues(const u8* image_bytes,
	Image_Information* image_information,
	r32** prologues,
	r32** last_prologue_contributions,
	s32 parallelism_level_x,
	s32 parallelism_level_y,
	const Block* image_blocks,
	s32* vertical_domain_transforms,
	r32** rf_table,
	s32 iteration,
	s32 number_of_threads);

extern void calculate_blocks_from_prologues(const u8* image_bytes,
	Image_Information* image_information,
	r32** prologues,
	s32 parallelism_level_x,
	s32 parallelism_level_y,
	const Block* image_blocks,
	s32* vertical_domain_transforms,
	r32** rf_table,
	s32 iteration,
	s32 number_of_threads,
	u8* image_result);

extern void calculate_complete_prologues(Image_Information* image_information,
	r32** complete_prologues,
	r32** incomplete_prologues,
	r32** last_prologue_contributions,
	Block* image_blocks,
	s32 parallelism_level_x,
	s32 parallelism_level_y);