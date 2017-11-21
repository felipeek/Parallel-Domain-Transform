#include "EpilogueT.h"

extern void calculate_complete_epiloguesT(Image_Information* image_information,
	r32** complete_epiloguesT,
	r32** incomplete_epiloguesT,
	r32** last_epilogueT_contributions,
	Block* image_blocks,
	s32 parallelism_level_x,
	s32 parallelism_level_y)
{
	for (s32 i = 0; i < parallelism_level_y; ++i)
	{
		for (s32 j = parallelism_level_x - 1; j >= 0; --j)
			// @TODO: This last 'for' can be parallelized
			for (s32 k = 0; k < (image_blocks + i * parallelism_level_x + j)->height; ++k)
			{
				R32_Color last_epilogueT;

				if (j != parallelism_level_x - 1)
				{
					last_epilogueT = {
						complete_epiloguesT[i * parallelism_level_x + j + 1][k * image_information->channels + 0],
						complete_epiloguesT[i * parallelism_level_x + j + 1][k * image_information->channels + 1],
						complete_epiloguesT[i * parallelism_level_x + j + 1][k * image_information->channels + 2]
					};
				}
				else
				{
					last_epilogueT = { 0.0f, 0.0f, 0.0f };
				}

				// R Channel
				complete_epiloguesT[i * parallelism_level_x + j][k * image_information->channels + 0] =
					incomplete_epiloguesT[i * parallelism_level_x + j][k * image_information->channels + 0] +
					last_epilogueT_contributions[i * parallelism_level_x + j][k] * last_epilogueT.r;
				// G Channel
				complete_epiloguesT[i * parallelism_level_x + j][k * image_information->channels + 1] =
					incomplete_epiloguesT[i * parallelism_level_x + j][k * image_information->channels + 1] +
					last_epilogueT_contributions[i * parallelism_level_x + j][k] * last_epilogueT.g;
				// B Channel
				complete_epiloguesT[i * parallelism_level_x + j][k * image_information->channels + 2] =
					incomplete_epiloguesT[i * parallelism_level_x + j][k * image_information->channels + 2] +
					last_epilogueT_contributions[i * parallelism_level_x + j][k] * last_epilogueT.b;
			}
	}
}

intern Thread_Proc_Return_Type _stdcall fill_blocks_from_epilogues_thread_proc(void* thread_information)
{
	Thread_Block_EpilogueT_Information* block_information = (Thread_Block_EpilogueT_Information*)thread_information;

	for (s32 j = block_information->block->y, count = 0; j < block_information->block->y + block_information->block->height; ++j, ++count)
	{
		R32_Color last_pixel = { 0.0f, 0.0f, 0.0f };
		R32_Color current_pixel = { 0.0f, 0.0f, 0.0f };

		if (block_information->last_epiloguesT)
		{
			last_pixel = {
				block_information->last_epiloguesT[count * block_information->image_information->channels],
				block_information->last_epiloguesT[count * block_information->image_information->channels + 1],
				block_information->last_epiloguesT[count * block_information->image_information->channels + 2],
			};
		}

		for (s32 i = block_information->block->x + block_information->block->width - 1; i >= block_information->block->x; --i)
		{
			s32 d_x_position = (i < block_information->image_information->width - 1) ? i + 1 : i;
			s32 d = block_information->domain_transforms[j * block_information->image_information->width + d_x_position];
			r32 w = (block_information->rf_table)[block_information->iteration][d];
			current_pixel.r = block_information->image_bytes[j * block_information->image_information->width *
				block_information->image_information->channels + i * block_information->image_information->channels];
			current_pixel.g = block_information->image_bytes[j * block_information->image_information->width *
				block_information->image_information->channels + i * block_information->image_information->channels + 1];
			current_pixel.b = block_information->image_bytes[j * block_information->image_information->width *
				block_information->image_information->channels + i * block_information->image_information->channels + 2];
			last_pixel.r = ((1 - w) * current_pixel.r + w * last_pixel.r);
			last_pixel.g = ((1 - w) * current_pixel.g + w * last_pixel.g);
			last_pixel.b = ((1 - w) * current_pixel.b + w * last_pixel.b);

			block_information->image_result[j * block_information->image_information->width *
				block_information->image_information->channels + i * block_information->image_information->channels] = last_pixel.r;
			block_information->image_result[j * block_information->image_information->width *
				block_information->image_information->channels + i * block_information->image_information->channels + 1] = last_pixel.g;
			block_information->image_result[j * block_information->image_information->width *
				block_information->image_information->channels + i * block_information->image_information->channels + 2] = last_pixel.b;
			block_information->image_result[j * block_information->image_information->width *
				block_information->image_information->channels + i * block_information->image_information->channels + 3] = 255;
		}
	}

	return 0;
}

extern void calculate_blocks_from_epiloguesT(const r32* image_bytes,
	Image_Information* image_information,
	r32** epiloguesT,
	s32 parallelism_level_x,
	s32 parallelism_level_y,
	const Block* image_blocks,
	s32* vertical_domain_transforms,
	r32** rf_table,
	s32 iteration,
	s32 number_of_threads,
	r32* image_result,
	Thread_Block_EpilogueT_Information* thread_informations_memory,
	Thread_Handler* active_threads_memory)
{
	// Fill Thread Informations
	for (s32 i = 0; i < parallelism_level_y; ++i)
	{
		r32* last_epiloguesT = 0;

		for (s32 j = parallelism_level_x - 1; j >= 0; --j)
		{
			thread_informations_memory[i * parallelism_level_x + j].block = image_blocks + i * parallelism_level_x + j;
			thread_informations_memory[i * parallelism_level_x + j].image_bytes = image_bytes;
			thread_informations_memory[i * parallelism_level_x + j].image_information = image_information;
			thread_informations_memory[i * parallelism_level_x + j].domain_transforms = vertical_domain_transforms;
			thread_informations_memory[i * parallelism_level_x + j].rf_table = rf_table;
			thread_informations_memory[i * parallelism_level_x + j].iteration = iteration;
			thread_informations_memory[i * parallelism_level_x + j].last_epiloguesT = last_epiloguesT;
			thread_informations_memory[i * parallelism_level_x + j].image_result = image_result;

			last_epiloguesT = epiloguesT[i * parallelism_level_x + j];
		}
	}
	
	s32 num_active_threads = 0;

	// Calculate Blocks
	for (s32 i = 0; i < parallelism_level_y; ++i)
	{
		for (s32 j = 0; j < parallelism_level_x; ++j)
		{
			active_threads_memory[num_active_threads++] = create_thread(fill_blocks_from_epilogues_thread_proc, thread_informations_memory + i * parallelism_level_x + j);

			if (num_active_threads == number_of_threads)
			{
				join_threads(num_active_threads, active_threads_memory, 1);
				num_active_threads = 0;
			}
		}
	}

	join_threads(num_active_threads, active_threads_memory, 1);
}

intern Thread_Proc_Return_Type _stdcall fill_incomplete_epilogues_thread_proc(void* thread_information)
{
	Thread_Incomplete_EpilogueT_Information* epilogueT_information = (Thread_Incomplete_EpilogueT_Information*)thread_information;

	for (s32 j = epilogueT_information->block->y, count = 0; j < epilogueT_information->block->y + epilogueT_information->block->height; ++j, ++count)
	{
		R32_Color last_pixel = { 0.0f, 0.0f, 0.0f };
		R32_Color current_pixel = { 0.0f, 0.0f, 0.0f };

		epilogueT_information->last_epilogueT_contribution[count] = 1;

		for (s32 i = epilogueT_information->block->x + epilogueT_information->block->width - 1; i >= epilogueT_information->block->x ; --i)
		{
			s32 d_x_position = (i < epilogueT_information->image_information->width - 1) ? i + 1 : i;
			s32 d = epilogueT_information->domain_transforms[j * epilogueT_information->image_information->width + d_x_position];
			r32 w = (epilogueT_information->rf_table)[epilogueT_information->iteration][d];
			current_pixel.r = epilogueT_information->image_bytes[j * epilogueT_information->image_information->width *
				epilogueT_information->image_information->channels + i * epilogueT_information->image_information->channels];
			current_pixel.g = epilogueT_information->image_bytes[j * epilogueT_information->image_information->width *
				epilogueT_information->image_information->channels + i * epilogueT_information->image_information->channels + 1];
			current_pixel.b = epilogueT_information->image_bytes[j * epilogueT_information->image_information->width *
				epilogueT_information->image_information->channels + i * epilogueT_information->image_information->channels + 2];
			last_pixel.r = ((1 - w) * current_pixel.r + w * last_pixel.r);
			last_pixel.g = ((1 - w) * current_pixel.g + w * last_pixel.g);
			last_pixel.b = ((1 - w) * current_pixel.b + w * last_pixel.b);

			epilogueT_information->last_epilogueT_contribution[count] *= w;
		}

		epilogueT_information->incomplete_epilogueT[count * epilogueT_information->image_information->channels + 0] = last_pixel.r;
		epilogueT_information->incomplete_epilogueT[count * epilogueT_information->image_information->channels + 1] = last_pixel.g;
		epilogueT_information->incomplete_epilogueT[count * epilogueT_information->image_information->channels + 2] = last_pixel.b;
		epilogueT_information->incomplete_epilogueT[count * epilogueT_information->image_information->channels + 3] = 0;
	}

	return 0;
}

extern void calculate_incomplete_epiloguesT(const r32* image_bytes,
	Image_Information* image_information,
	r32** epiloguesT,
	r32** last_epilogueT_contributions,
	s32 parallelism_level_x,
	s32 parallelism_level_y,
	const Block* image_blocks,
	s32* vertical_domain_transforms,
	r32** rf_table,
	s32 iteration,
	s32 number_of_threads,
	Thread_Incomplete_EpilogueT_Information* thread_informations_memory,
	Thread_Handler* active_threads_memory)
{
	// Fill Thread Informations
	for (s32 i = 0; i < parallelism_level_y; ++i)
		for (s32 j = 0; j < parallelism_level_x; ++j)
		{
			thread_informations_memory[i * parallelism_level_x + j].block = image_blocks + i * parallelism_level_x + j;
			thread_informations_memory[i * parallelism_level_x + j].image_bytes = image_bytes;
			thread_informations_memory[i * parallelism_level_x + j].image_information = image_information;
			thread_informations_memory[i * parallelism_level_x + j].domain_transforms = vertical_domain_transforms;
			thread_informations_memory[i * parallelism_level_x + j].rf_table = rf_table;
			thread_informations_memory[i * parallelism_level_x + j].iteration = iteration;
			thread_informations_memory[i * parallelism_level_x + j].last_epilogueT_contribution = last_epilogueT_contributions[i * parallelism_level_x + j];
			thread_informations_memory[i * parallelism_level_x + j].incomplete_epilogueT = epiloguesT[i * parallelism_level_x + j];
		}

	s32 num_active_threads = 0;

	// Calculate Blocks
	for (s32 i = 0; i < parallelism_level_y; ++i)
	{
		for (s32 j = 0; j < parallelism_level_x; ++j)
		{
			active_threads_memory[num_active_threads++] = create_thread(fill_incomplete_epilogues_thread_proc, thread_informations_memory + i * parallelism_level_x + j);

			if (num_active_threads == number_of_threads)
			{
				join_threads(num_active_threads, active_threads_memory, 1);
				num_active_threads = 0;
			}
		}
	}

	join_threads(num_active_threads, active_threads_memory, 1);
}
