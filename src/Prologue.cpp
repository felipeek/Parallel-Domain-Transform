#include "Prologue.h"

extern void calculate_complete_prologues(Image_Information* image_information,
	r32** complete_prologues,
	r32** incomplete_prologues,
	r32** last_prologue_contributions,
	Block* image_blocks,
	s32 parallelism_level_x,
	s32 parallelism_level_y)
{
	for (s32 j = 0; j < parallelism_level_x; ++j)
	{
		for (s32 i = 0; i < parallelism_level_y; ++i)
			// @TODO: This last 'for' can be parallelized
			for (s32 k = 0; k < (image_blocks + i * parallelism_level_x + j)->width; ++k)
			{
				R32_Color last_prologue;

				if (i != 0)
				{
					last_prologue = {
						complete_prologues[(i - 1) * parallelism_level_x + j][k * image_information->channels + 0],
						complete_prologues[(i - 1) * parallelism_level_x + j][k * image_information->channels + 1],
						complete_prologues[(i - 1) * parallelism_level_x + j][k * image_information->channels + 2]
					};
				}
				else
				{
					last_prologue = { 0.0f, 0.0f, 0.0f };
				}

				// R Channel
				complete_prologues[i * parallelism_level_x + j][k * image_information->channels + 0] =
					incomplete_prologues[i * parallelism_level_x + j][k * image_information->channels + 0] +
					last_prologue_contributions[i * parallelism_level_x + j][k] * last_prologue.r;
				// G Channel
				complete_prologues[i * parallelism_level_x + j][k * image_information->channels + 1] =
					incomplete_prologues[i * parallelism_level_x + j][k * image_information->channels + 1] +
					last_prologue_contributions[i * parallelism_level_x + j][k] * last_prologue.g;
				// B Channel
				complete_prologues[i * parallelism_level_x + j][k * image_information->channels + 2] =
					incomplete_prologues[i * parallelism_level_x + j][k * image_information->channels + 2] +
					last_prologue_contributions[i * parallelism_level_x + j][k] * last_prologue.b;
			}
	}
}

static Thread_Proc_Return_Type _stdcall fill_blocks_from_prologues_thread_proc(void* thread_information)
{
	Thread_Block_Prologue_Information* block_information = (Thread_Block_Prologue_Information*)thread_information;

	for (s32 i = block_information->block->x, count = 0; i < block_information->block->x + block_information->block->width; ++i, ++count)
	{
		R32_Color last_pixel = { 0.0f, 0.0f, 0.0f };
		R32_Color current_pixel = { 0.0f, 0.0f, 0.0f };

		if (block_information->last_prologues)
		{
			last_pixel = {
				block_information->last_prologues[count * block_information->image_information->channels],
				block_information->last_prologues[count * block_information->image_information->channels + 1],
				block_information->last_prologues[count * block_information->image_information->channels + 2],
			};
		}
		else
		{
			last_pixel = {
				block_information->image_bytes[block_information->block->y * block_information->image_information->width *
					block_information->image_information->channels + i * block_information->image_information->channels],
				block_information->image_bytes[block_information->block->y * block_information->image_information->width *
					block_information->image_information->channels + i * block_information->image_information->channels + 1],
				block_information->image_bytes[block_information->block->y * block_information->image_information->width *
					block_information->image_information->channels + i * block_information->image_information->channels + 2],
			};
		}

		for (s32 j = block_information->block->y; j < block_information->block->y + block_information->block->height; ++j)
		{
			s32 d = block_information->domain_transforms[j * block_information->image_information->width + i];
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
				block_information->image_information->channels + i * block_information->image_information->channels + 3] = 1.0f;
		}
	}

	return 0;
}

extern void calculate_blocks_from_prologues(const r32* image_bytes,
	Image_Information* image_information,
	r32** prologues,
	s32 parallelism_level_x,
	s32 parallelism_level_y,
	const Block* image_blocks,
	s32* vertical_domain_transforms,
	r32** rf_table,
	s32 iteration,
	s32 number_of_threads,
	r32* image_result,
	Thread_Block_Prologue_Information* thread_informations_memory,
	Thread_Handler* active_threads_memory)
{
	// Fill Thread Informations
	for (s32 j = 0; j < parallelism_level_x; ++j)
	{
		r32* last_prologues = 0;

		for (s32 i = 0; i < parallelism_level_y; ++i)
		{
			thread_informations_memory[i * parallelism_level_x + j].block = image_blocks + i * parallelism_level_x + j;
			thread_informations_memory[i * parallelism_level_x + j].image_bytes = image_bytes;
			thread_informations_memory[i * parallelism_level_x + j].image_information = image_information;
			thread_informations_memory[i * parallelism_level_x + j].domain_transforms = vertical_domain_transforms;
			thread_informations_memory[i * parallelism_level_x + j].rf_table = rf_table;
			thread_informations_memory[i * parallelism_level_x + j].iteration = iteration;
			thread_informations_memory[i * parallelism_level_x + j].last_prologues = last_prologues;
			thread_informations_memory[i * parallelism_level_x + j].image_result = image_result;

			last_prologues = prologues[i * parallelism_level_x + j];
		}
	}

	s32 num_active_threads = 0;

	// Calculate Blocks
	for (s32 i = 0; i < parallelism_level_y; ++i)
	{
		for (s32 j = 0; j < parallelism_level_x; ++j)
		{
			active_threads_memory[num_active_threads++] = create_thread(fill_blocks_from_prologues_thread_proc, thread_informations_memory + i * parallelism_level_x + j);

			if (num_active_threads == number_of_threads)
			{
				join_threads(num_active_threads, active_threads_memory, 1);
				num_active_threads = 0;
			}
		}
	}

	join_threads(num_active_threads, active_threads_memory, 1);
}

static Thread_Proc_Return_Type _stdcall fill_incomplete_prologues_thread_proc(void* thread_information)
{
	Thread_Incomplete_Prologue_Information* prologue_information = (Thread_Incomplete_Prologue_Information*)thread_information;

	for (s32 i = prologue_information->block->x, count = 0; i < prologue_information->block->x + prologue_information->block->width; ++i, ++count)
	{
		R32_Color last_pixel = { 0.0f, 0.0f, 0.0f };
		R32_Color current_pixel = { 0.0f, 0.0f, 0.0f };

		prologue_information->last_prologue_contribution[count] = 1;

		for (s32 j = prologue_information->block->y; j < prologue_information->block->y + prologue_information->block->height; ++j)
		{
			s32 d = prologue_information->domain_transforms[j * prologue_information->image_information->width + i];
			r32 w = (prologue_information->rf_table)[prologue_information->iteration][d];
			current_pixel.r = prologue_information->image_bytes[j * prologue_information->image_information->width *
				prologue_information->image_information->channels + i * prologue_information->image_information->channels];
			current_pixel.g = prologue_information->image_bytes[j * prologue_information->image_information->width *
				prologue_information->image_information->channels + i * prologue_information->image_information->channels + 1];
			current_pixel.b = prologue_information->image_bytes[j * prologue_information->image_information->width *
				prologue_information->image_information->channels + i * prologue_information->image_information->channels + 2];
			last_pixel.r = ((1 - w) * current_pixel.r + w * last_pixel.r);
			last_pixel.g = ((1 - w) * current_pixel.g + w * last_pixel.g);
			last_pixel.b = ((1 - w) * current_pixel.b + w * last_pixel.b);

			prologue_information->last_prologue_contribution[count] *= w;
		}

		prologue_information->incomplete_prologue[count * prologue_information->image_information->channels + 0] = last_pixel.r;
		prologue_information->incomplete_prologue[count * prologue_information->image_information->channels + 1] = last_pixel.g;
		prologue_information->incomplete_prologue[count * prologue_information->image_information->channels + 2] = last_pixel.b;
		prologue_information->incomplete_prologue[count * prologue_information->image_information->channels + 3] = 0;
	}

	return 0;
}

extern void calculate_incomplete_prologues(const r32* image_bytes,
	Image_Information* image_information,
	r32** prologues,
	r32** last_prologue_contributions,
	s32 parallelism_level_x,
	s32 parallelism_level_y,
	const Block* image_blocks,
	s32* vertical_domain_transforms,
	r32** rf_table,
	s32 iteration,
	s32 number_of_threads,
	Thread_Incomplete_Prologue_Information* thread_informations_memory,
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
			thread_informations_memory[i * parallelism_level_x + j].last_prologue_contribution = last_prologue_contributions[i * parallelism_level_x + j];
			thread_informations_memory[i * parallelism_level_x + j].incomplete_prologue = prologues[i * parallelism_level_x + j];
		}

	s32 num_active_threads = 0;

	// Calculate Blocks
	for (s32 i = 0; i < parallelism_level_y; ++i)
	{
		for (s32 j = 0; j < parallelism_level_x; ++j)
		{
			active_threads_memory[num_active_threads++] = create_thread(fill_incomplete_prologues_thread_proc, thread_informations_memory + i * parallelism_level_x + j);

			if (num_active_threads == number_of_threads)
			{
				join_threads(num_active_threads, active_threads_memory, 1);
				num_active_threads = 0;
			}
		}
	}

	join_threads(num_active_threads, active_threads_memory, 1);
}
