#include "ParallelDomainTransform.h"
#include "Util.h"
#include "Prologue.h"
#include "Epilogue.h"
#include "PrologueT.h"
#include "EpilogueT.h"

// This should be 768, since we have 2^8 possible options for each channels.
// Since domain transform is given by:
// 1 + (s_s / s_r) * (diff_channel_R + diff_channel_G + diff_channel_B)
// We will have 3 * 2^8 different possibilities for the transform of each pixel.
// 3 * 2^8 = 768
#define RF_TABLE_SIZE 768
#define SQRT3 1.7320508075f
#define SQRT2 1.4142135623f
#define IN
#define OUT

typedef struct Struct_Thread_DT_Information Thread_DT_Information;

struct Struct_Thread_DT_Information
{
	IN const Block* block;
	IN const r32* image_bytes;
	IN const Image_Information* image_information;
	OUT s32* vertical_domain_transforms;
	OUT s32* horizontal_domain_transforms;
};

intern s32 _stdcall fill_block_domain_transforms_thread_proc(void* thread_dt_information)
{
	Thread_DT_Information* vdt_information = (Thread_DT_Information*)thread_dt_information;
	R32_Color last_pixel;
	R32_Color current_pixel;
	r32 sum_channels_difference;
	s32 d;

	// Fill Horizontal Domain Transforms
	for (s32 i = vdt_information->block->y; i < vdt_information->block->y + vdt_information->block->height; ++i)
	{
		if (vdt_information->block->x != 0)
		{
			last_pixel.r = *(vdt_information->image_bytes + i * vdt_information->image_information->width * vdt_information->image_information->channels +
				(vdt_information->block->x - 1) * vdt_information->image_information->channels);
			last_pixel.g = *(vdt_information->image_bytes + i * vdt_information->image_information->width * vdt_information->image_information->channels +
				(vdt_information->block->x - 1) * vdt_information->image_information->channels + 1);
			last_pixel.b = *(vdt_information->image_bytes + i * vdt_information->image_information->width * vdt_information->image_information->channels +
				(vdt_information->block->x - 1) * vdt_information->image_information->channels + 2);
		}
		else
		{
			last_pixel.r = *(vdt_information->image_bytes + i * vdt_information->image_information->width * vdt_information->image_information->channels +
				vdt_information->block->x * vdt_information->image_information->channels);
			last_pixel.g = *(vdt_information->image_bytes + i * vdt_information->image_information->width * vdt_information->image_information->channels +
				vdt_information->block->x * vdt_information->image_information->channels + 1);
			last_pixel.b = *(vdt_information->image_bytes + i * vdt_information->image_information->width * vdt_information->image_information->channels +
				vdt_information->block->x * vdt_information->image_information->channels + 2);
		}

		for (s32 j = vdt_information->block->x; j < vdt_information->block->x + vdt_information->block->width; ++j)
		{
			current_pixel.r = *(vdt_information->image_bytes + i * vdt_information->image_information->width * vdt_information->image_information->channels +
				j * vdt_information->image_information->channels);
			current_pixel.g = *(vdt_information->image_bytes + i * vdt_information->image_information->width * vdt_information->image_information->channels +
				j * vdt_information->image_information->channels + 1);
			current_pixel.b = *(vdt_information->image_bytes + i * vdt_information->image_information->width * vdt_information->image_information->channels +
				j * vdt_information->image_information->channels + 2);

			sum_channels_difference = absolute(current_pixel.r - last_pixel.r) + absolute(current_pixel.g - last_pixel.g) + absolute(current_pixel.b - last_pixel.b);

			// @NOTE: 'd' should be 1.0f + s_s / s_r * sum_diff
			// However, we will store just sum_diff.
			// 1.0f + s_s / s_r will be calculated later when calculating the RF table.
			// This is done this way because the sum_diff is perfect to be used as the index of the RF table.
			// d = 1.0f + (vdt_information->spatial_factor / vdt_information->range_factor) * sum_channels_difference;
			d = (s32)r32_round(sum_channels_difference * 255.0f);

			*(vdt_information->horizontal_domain_transforms + i * vdt_information->image_information->width + j) = d;

			last_pixel.r = current_pixel.r;
			last_pixel.g = current_pixel.g;
			last_pixel.b = current_pixel.b;
		}
	}

	// Fill Vertical Domain Transforms
	for (s32 i = vdt_information->block->x; i < vdt_information->block->x + vdt_information->block->width; ++i)
	{
		if (vdt_information->block->y != 0)
		{
			last_pixel.r = *(vdt_information->image_bytes + (vdt_information->block->y - 1) * vdt_information->image_information->width *
				vdt_information->image_information->channels + i * vdt_information->image_information->channels);
			last_pixel.g = *(vdt_information->image_bytes + (vdt_information->block->y - 1) * vdt_information->image_information->width *
				vdt_information->image_information->channels + i * vdt_information->image_information->channels + 1);
			last_pixel.b = *(vdt_information->image_bytes + (vdt_information->block->y - 1) * vdt_information->image_information->width *
				vdt_information->image_information->channels + i * vdt_information->image_information->channels + 2);
		}
		else
		{
			last_pixel.r = *(vdt_information->image_bytes + vdt_information->block->y * vdt_information->image_information->width *
				vdt_information->image_information->channels + i * vdt_information->image_information->channels);
			last_pixel.g = *(vdt_information->image_bytes + vdt_information->block->y * vdt_information->image_information->width *
				vdt_information->image_information->channels + i * vdt_information->image_information->channels + 1);
			last_pixel.b = *(vdt_information->image_bytes + vdt_information->block->y * vdt_information->image_information->width *
				vdt_information->image_information->channels + i * vdt_information->image_information->channels + 2);
		}

		for (s32 j = vdt_information->block->y; j < vdt_information->block->y + vdt_information->block->height; ++j)
		{
			current_pixel.r = *(vdt_information->image_bytes + j * vdt_information->image_information->width * vdt_information->image_information->channels +
				i * vdt_information->image_information->channels);
			current_pixel.g = *(vdt_information->image_bytes + j * vdt_information->image_information->width * vdt_information->image_information->channels +
				i * vdt_information->image_information->channels + 1);
			current_pixel.b = *(vdt_information->image_bytes + j * vdt_information->image_information->width * vdt_information->image_information->channels +
				i * vdt_information->image_information->channels + 2);

			sum_channels_difference = absolute(current_pixel.r - last_pixel.r) + absolute(current_pixel.g - last_pixel.g) + absolute(current_pixel.b - last_pixel.b);

			// @NOTE: 'd' should be 1.0f + s_s / s_r * sum_diff
			// However, we will store just sum_diff.
			// 1.0f + s_s / s_r will be calculated later when calculating the RF table.
			// This is done this way because the sum_diff is perfect to be used as the index of the RF table.
			// d = 1.0f + (vdt_information->spatial_factor / vdt_information->range_factor) * sum_channels_difference;
			d = (s32)r32_round(sum_channels_difference * 255.0f);

			*(vdt_information->vertical_domain_transforms + j * vdt_information->image_information->width + i) = d;
			
			last_pixel.r = current_pixel.r;
			last_pixel.g = current_pixel.g;
			last_pixel.b = current_pixel.b;
		}
	}

	return 0;
}

intern void fill_domain_transforms(s32* vertical_domain_transforms,
	s32* horizontal_domain_transforms,
	const r32* image_bytes,
	const Image_Information* image_information,
	r32 spatial_factor,
	r32 range_factor,
	s32 number_of_threads,
	s32 parallelism_level_x,
	s32 parallelism_level_y,
	const Block* image_blocks)
{
	Thread_DT_Information* dt_informations = (Thread_DT_Information*)alloc_memory(sizeof(Thread_DT_Information) * parallelism_level_x * parallelism_level_y);

	// Fill DT Informations
	for (s32 i = 0; i < parallelism_level_y; ++i)
	{
		for (s32 j = 0; j < parallelism_level_x; ++j)
		{
			(dt_informations + i * parallelism_level_x + j)->block = image_blocks + i * parallelism_level_x + j;
			(dt_informations + i * parallelism_level_x + j)->image_bytes = image_bytes;
			(dt_informations + i * parallelism_level_x + j)->image_information = image_information;
			(dt_informations + i * parallelism_level_x + j)->vertical_domain_transforms = vertical_domain_transforms;
			(dt_informations + i * parallelism_level_x + j)->horizontal_domain_transforms = horizontal_domain_transforms;
		}
	}

	void** active_threads = (void**)alloc_memory(sizeof(void*) * number_of_threads);
	s32 num_active_threads = 0;

	// Fill Domain Transforms
	for (s32 i = 0; i < parallelism_level_y; ++i)
	{
		for (s32 j = 0; j < parallelism_level_x; ++j)
		{
			active_threads[num_active_threads++] = create_thread(fill_block_domain_transforms_thread_proc, dt_informations + i * parallelism_level_x + j);
			if (num_active_threads == number_of_threads)
			{
				join_threads(num_active_threads, active_threads, 1);
				num_active_threads = 0;
			}
		}
	}

	join_threads(num_active_threads, active_threads, 1);

	dealloc_memory(active_threads);
}

intern void paint_block(r32* img_data,
	Image_Information* img,
	const Block* b,
	const R32_Color* c)
{
	for (s32 i = b->y; i < b->y + b->height; ++i)
	{
		for (s32 j = b->x; j < b->x + b->width; ++j)
		{
			*(img_data + i * img->width * img->channels + j * img->channels) = c->r;
			*(img_data + i * img->width * img->channels + j * img->channels + 1) = c->g;
			*(img_data + i * img->width * img->channels + j * img->channels + 2) = c->b;
		}
	}
}

extern s32 parallel_domain_transform(const r32* image_bytes,
	s32 image_width,
	s32 image_height,
	s32 image_channels,
	r32 spatial_factor,
	r32 range_factor,
	s32 num_iterations,
	s32 number_of_threads,
	s32 parallelism_level_x,
	s32 parallelism_level_y,
	r32* image_result)
{
	Image_Information image_information;

	image_information.height = image_height;
	image_information.width = image_width;
	image_information.channels = image_channels;

	// Separate image in blocks.
	Block* image_blocks = (Block*)alloc_memory(sizeof(Block) * parallelism_level_x * parallelism_level_y);
	s32 blocks_accumulate_x = 0, blocks_accumulate_y = 0;
	s32 current_block_width, current_block_height;

	for (s32 i = 0; i < parallelism_level_y; ++i)
	{
		current_block_height = (image_height - blocks_accumulate_y) / (parallelism_level_y - i);

		for (s32 j = 0; j < parallelism_level_x; ++j)
		{
			current_block_width = (image_width - blocks_accumulate_x) / (parallelism_level_x - j);

			Block b;
			b.x = blocks_accumulate_x;
			b.y = blocks_accumulate_y;
			b.width = current_block_width;
			b.height = current_block_height;
			*(image_blocks + i * parallelism_level_x + j) = b;

			blocks_accumulate_x = blocks_accumulate_x + current_block_width;
		}

		blocks_accumulate_y = blocks_accumulate_y + current_block_height;
		blocks_accumulate_x = 0;
	}

	// Pre-calculate RF feedback coefficients
	r32* rf_coefficients = (r32*)alloc_memory(sizeof(r32) * num_iterations);

	for (s32 i = 0; i < num_iterations; ++i)
	{
		// calculating RF feedback coefficient 'a' from desired variance
		// 'a' will change each iteration while the domain transform will remain constant
		r32 current_standard_deviation = spatial_factor * SQRT3 * (r32_pow(2.0f, (r32)(num_iterations - (i + 1))) / r32_sqrt(r32_pow(4.0f, (r32)num_iterations) - 1));
		r32 a = r32_exp(-SQRT2 / current_standard_deviation);

		rf_coefficients[i] = a;
	}

	// Pre-calculate RF table
	r32** rf_table = (r32**)alloc_memory(sizeof(r32*) * num_iterations);
	r32 d;
	
	for (s32 i = 0; i < num_iterations; ++i)
	{
		rf_table[i] = (r32*)alloc_memory(sizeof(r32) * RF_TABLE_SIZE);

		for (s32 j = 0; j < RF_TABLE_SIZE; ++j)
		{
			d = 1.0f + (spatial_factor / range_factor) * j;
			rf_table[i][j] = r32_pow(rf_coefficients[i], d);
		}
	}

	// Pre-calculate Domain Transform
	s32* vertical_domain_transforms = (s32*)alloc_memory(sizeof(s32) * image_height * image_width);
	s32* horizontal_domain_transforms = (s32*)alloc_memory(sizeof(s32) * image_height * image_width);

	fill_domain_transforms(vertical_domain_transforms,
		horizontal_domain_transforms,
		image_result,
		&image_information,
		spatial_factor,
		range_factor,
		number_of_threads,
		parallelism_level_x,
		parallelism_level_y,
		image_blocks);

	// Filter Iterations
	for (s32 i = 0; i < num_iterations; ++i)
	{
		// Calculate Incomplete Prologues
		r32** prologues = (r32**)alloc_memory(sizeof(r32*) * parallelism_level_x * parallelism_level_y);
		r32** last_prologue_contributions = (r32**)alloc_memory(sizeof(r32*) * parallelism_level_x * parallelism_level_y);
		
		// if first iteration, send image_bytes.
		if (i == 0)
			calculate_incomplete_prologues(image_bytes,
				&image_information,
				prologues,
				last_prologue_contributions,
				parallelism_level_x,
				parallelism_level_y,
				image_blocks,
				vertical_domain_transforms,
				rf_table,
				i,
				number_of_threads);
		else
			calculate_incomplete_prologues(image_result,
				&image_information,
				prologues,
				last_prologue_contributions,
				parallelism_level_x,
				parallelism_level_y,
				image_blocks,
				vertical_domain_transforms,
				rf_table,
				i,
				number_of_threads);
		
		// Calculate Complete Prologues
		calculate_complete_prologues(&image_information,
			prologues,
			prologues,
			last_prologue_contributions,
			image_blocks,
			parallelism_level_x,
			parallelism_level_y);
		
		// Calculate Full Blocks From Prologues
		calculate_blocks_from_prologues(image_result,
			&image_information,
			prologues,
			parallelism_level_x,
			parallelism_level_y,
			image_blocks,
			vertical_domain_transforms,
			rf_table,
			i,
			number_of_threads,
			image_result);
		
		// Dealloc memory
		for (s32 i = 0; i < parallelism_level_y; ++i)
			for (s32 j = 0; j < parallelism_level_x; ++j)
				dealloc_memory(*(prologues + i * parallelism_level_x + j));
		dealloc_memory(prologues);
		
		for (s32 i = 0; i < parallelism_level_y; ++i)
			for (s32 j = 0; j < parallelism_level_x; ++j)
				dealloc_memory(*(last_prologue_contributions + i * parallelism_level_x + j));
		dealloc_memory(last_prologue_contributions);

		// Calculate Incomplete Epilogues
		r32** epilogues = (r32**)alloc_memory(sizeof(r32*) * parallelism_level_x * parallelism_level_y);
		r32** last_epilogue_contributions = (r32**)alloc_memory(sizeof(r32*) * parallelism_level_x * parallelism_level_y);
		
		calculate_incomplete_epilogues(image_result,
			&image_information,
			epilogues,
			last_epilogue_contributions,
			parallelism_level_x,
			parallelism_level_y,
			image_blocks,
			vertical_domain_transforms,
			rf_table,
			i,
			number_of_threads);
		
		// Calculate Complete Epilogues
		calculate_complete_epilogues(&image_information,
			epilogues,
			epilogues,
			last_epilogue_contributions,
			image_blocks,
			parallelism_level_x,
			parallelism_level_y);
		
		// Calculate Full Blocks From Epilogues
		calculate_blocks_from_epilogues(image_result,
			&image_information,
			epilogues,
			parallelism_level_x,
			parallelism_level_y,
			image_blocks,
			vertical_domain_transforms,
			rf_table,
			i,
			number_of_threads,
			image_result);
		
		// Dealloc memory
		for (s32 i = 0; i < parallelism_level_y; ++i)
			for (s32 j = 0; j < parallelism_level_x; ++j)
				dealloc_memory(*(epilogues + i * parallelism_level_x + j));
		dealloc_memory(epilogues);
		
		for (s32 i = 0; i < parallelism_level_y; ++i)
			for (s32 j = 0; j < parallelism_level_x; ++j)
				dealloc_memory(*(last_epilogue_contributions + i * parallelism_level_x + j));
		dealloc_memory(last_epilogue_contributions);
		
		//// Calculate Incomplete ProloguesT
		r32** prologuesT = (r32**)alloc_memory(sizeof(r32*) * parallelism_level_x * parallelism_level_y);
		r32** last_prologueT_contributions = (r32**)alloc_memory(sizeof(r32*) * parallelism_level_x * parallelism_level_y);
		
		calculate_incomplete_prologuesT(image_result,
			&image_information,
			prologuesT,
			last_prologueT_contributions,
			parallelism_level_x,
			parallelism_level_y,
			image_blocks,
			horizontal_domain_transforms,
			rf_table,
			i,
			number_of_threads);
		
		// Calculate Complete ProloguesT
		calculate_complete_prologuesT(&image_information,
			prologuesT,
			prologuesT,
			last_prologueT_contributions,
			image_blocks,
			parallelism_level_x,
			parallelism_level_y);
		
		// Calculate Full Blocks From ProloguesT
		calculate_blocks_from_prologuesT(image_result,
			&image_information,
			prologuesT,
			parallelism_level_x,
			parallelism_level_y,
			image_blocks,
			horizontal_domain_transforms,
			rf_table,
			i,
			number_of_threads,
			image_result);
		
		// Dealloc memory
		for (s32 i = 0; i < parallelism_level_y; ++i)
			for (s32 j = 0; j < parallelism_level_x; ++j)
				dealloc_memory(*(prologuesT + i * parallelism_level_x + j));
		dealloc_memory(prologuesT);
		
		for (s32 i = 0; i < parallelism_level_y; ++i)
			for (s32 j = 0; j < parallelism_level_x; ++j)
				dealloc_memory(*(last_prologueT_contributions + i * parallelism_level_x + j));
		dealloc_memory(last_prologueT_contributions);
		
		// Calculate Incomplete EpiloguesT
		r32** epiloguesT = (r32**)alloc_memory(sizeof(r32*) * parallelism_level_x * parallelism_level_y);
		r32** last_epilogueT_contributions = (r32**)alloc_memory(sizeof(r32*) * parallelism_level_x * parallelism_level_y);
		
		calculate_incomplete_epiloguesT(image_result,
			&image_information,
			epiloguesT,
			last_epilogueT_contributions,
			parallelism_level_x,
			parallelism_level_y,
			image_blocks,
			horizontal_domain_transforms,
			rf_table,
			i,
			number_of_threads);
		
		// Calculate Complete EpiloguesT
		calculate_complete_epiloguesT(&image_information,
			epiloguesT,
			epiloguesT,
			last_epilogueT_contributions,
			image_blocks,
			parallelism_level_x,
			parallelism_level_y);
		
		// Calculate Full Blocks From EpiloguesT
		calculate_blocks_from_epiloguesT(image_result,
			&image_information,
			epiloguesT,
			parallelism_level_x,
			parallelism_level_y,
			image_blocks,
			horizontal_domain_transforms,
			rf_table,
			i,
			number_of_threads,
			image_result);
		
		// Dealloc memory
		for (s32 i = 0; i < parallelism_level_y; ++i)
			for (s32 j = 0; j < parallelism_level_x; ++j)
				dealloc_memory(*(epiloguesT + i * parallelism_level_x + j));
		dealloc_memory(epiloguesT);
		
		for (s32 i = 0; i < parallelism_level_y; ++i)
			for (s32 j = 0; j < parallelism_level_x; ++j)
				dealloc_memory(*(last_epilogueT_contributions + i * parallelism_level_x + j));
		dealloc_memory(last_epilogueT_contributions);
	}
	

	// Dealloc memory
	for (s32 i = 0; i < num_iterations; ++i)
		dealloc_memory(rf_table[i]);
	dealloc_memory(rf_table);

	dealloc_memory(image_blocks);
	dealloc_memory(vertical_domain_transforms);
	dealloc_memory(horizontal_domain_transforms);

	return 0;
}