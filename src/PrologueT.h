#pragma once
#include "Common.h"

#define IN
#define OUT

typedef struct Struct_Thread_Incomplete_PrologueT_Information Thread_Incomplete_PrologueT_Information;
typedef struct Struct_Thread_Block_PrologueT_Information Thread_Block_PrologueT_Information;

struct Struct_Thread_Incomplete_PrologueT_Information
{
	IN const Block* block;
	IN const r32* image_bytes;
	IN const Image_Information* image_information;
	IN s32* domain_transforms;
	IN r32** rf_table;
	IN s32 iteration;
	OUT r32* last_prologueT_contribution;
	OUT r32* incomplete_prologueT;
};

struct Struct_Thread_Block_PrologueT_Information
{
	IN const Block* block;
	IN const r32* image_bytes;
	IN const Image_Information* image_information;
	IN s32* domain_transforms;
	IN r32** rf_table;
	IN s32 iteration;
	IN r32* last_prologuesT;
	OUT r32* image_result;
};

extern void calculate_incomplete_prologuesT(const r32* image_bytes,
	Image_Information* image_information,
	r32** prologuesT,
	r32** last_prologueT_contributions,
	s32 parallelism_level_x,
	s32 parallelism_level_y,
	const Block* image_blocks,
	s32* horizontal_domain_transforms,
	r32** rf_table,
	s32 iteration,
	s32 number_of_threads,
	Thread_Incomplete_PrologueT_Information* thread_informations_memory,
	void** active_threads_memory);

extern void calculate_blocks_from_prologuesT(const r32* image_bytes,
	Image_Information* image_information,
	r32** prologuesT,
	s32 parallelism_level_x,
	s32 parallelism_level_y,
	const Block* image_blocks,
	s32* horizontal_domain_transforms,
	r32** rf_table,
	s32 iteration,
	s32 number_of_threads,
	r32* image_result,
	Thread_Block_PrologueT_Information* thread_informations_memory,
	void** active_threads_memory);

extern void calculate_complete_prologuesT(Image_Information* image_information,
	r32** complete_prologuesT,
	r32** incomplete_prologuesT,
	r32** last_prologueT_contributions,
	Block* image_blocks,
	s32 parallelism_level_x,
	s32 parallelism_level_y);