#include "PrologueT.h"
#include "Util.h"

extern void calculate_complete_prologuesT(Image_Information* image_information,
	r32** complete_prologuesT,
	r32** incomplete_prologuesT,
	r32** last_prologueT_contributions,
	Block* image_blocks,
	s32 parallelism_level_x,
	s32 parallelism_level_y)
{
	for (s32 i = 0; i < parallelism_level_y; ++i)
	{
		for (s32 j = 0; j < parallelism_level_x; ++j)
			// @TODO: This last 'for' can be parallelized
			for (s32 k = 0; k < (image_blocks + i * parallelism_level_x + j)->height; ++k)
			{
				R32_Color last_prologueT;

				if (j != 0)
				{
					last_prologueT = {
						*(*(complete_prologuesT + i * parallelism_level_x + j - 1) + k * image_information->channels + 0),
						*(*(complete_prologuesT + i * parallelism_level_x + j - 1) + k * image_information->channels + 1),
						*(*(complete_prologuesT + i * parallelism_level_x + j - 1) + k * image_information->channels + 2)
					};
				}
				else
				{
					last_prologueT = { 0.0f, 0.0f, 0.0f };
				}

				// R Channel
				*(*(complete_prologuesT + i * parallelism_level_x + j) + k * image_information->channels + 0) =
					*(*(incomplete_prologuesT + i * parallelism_level_x + j) + k * image_information->channels + 0) +
					*(*(last_prologueT_contributions + i * parallelism_level_x + j) + k) * last_prologueT.r;
				// G Channel
				*(*(complete_prologuesT + i * parallelism_level_x + j) + k * image_information->channels + 1) =
					*(*(incomplete_prologuesT + i * parallelism_level_x + j) + k * image_information->channels + 1) +
					*(*(last_prologueT_contributions + i * parallelism_level_x + j) + k) * last_prologueT.g;
				// B Channel
				*(*(complete_prologuesT + i * parallelism_level_x + j) + k * image_information->channels + 2) =
					*(*(incomplete_prologuesT + i * parallelism_level_x + j) + k * image_information->channels + 2) +
					*(*(last_prologueT_contributions + i * parallelism_level_x + j) + k) * last_prologueT.b;
			}
	}
}

intern s32 _stdcall fill_blocks_from_prologuesT_thread_proc(void* thread_information)
{
	Thread_Block_PrologueT_Information* block_information = (Thread_Block_PrologueT_Information*)thread_information;

	for (s32 j = block_information->block->y, count = 0; j < block_information->block->y + block_information->block->height; ++j, ++count)
	{
		R32_Color last_pixel = { 0.0f, 0.0f, 0.0f };
		R32_Color current_pixel = { 0.0f, 0.0f, 0.0f };

		if (block_information->last_prologuesT)
		{
			last_pixel = {
				*(block_information->last_prologuesT + count * block_information->image_information->channels),
				*(block_information->last_prologuesT + count * block_information->image_information->channels + 1),
				*(block_information->last_prologuesT + count * block_information->image_information->channels + 2),
			};
		}

		for (s32 i = block_information->block->x; i < block_information->block->x + block_information->block->width; ++i)
		{
			s32 d = *(block_information->domain_transforms + j * block_information->image_information->width + i);
			r32 w = (block_information->rf_table)[block_information->iteration][d];
			current_pixel.r = *(block_information->image_bytes + j * block_information->image_information->width *
				block_information->image_information->channels + i * block_information->image_information->channels);
			current_pixel.g = *(block_information->image_bytes + j * block_information->image_information->width *
				block_information->image_information->channels + i * block_information->image_information->channels + 1);
			current_pixel.b = *(block_information->image_bytes + j * block_information->image_information->width *
				block_information->image_information->channels + i * block_information->image_information->channels + 2);
			last_pixel.r = ((1 - w) * current_pixel.r + w * last_pixel.r);
			last_pixel.g = ((1 - w) * current_pixel.g + w * last_pixel.g);
			last_pixel.b = ((1 - w) * current_pixel.b + w * last_pixel.b);

			*(block_information->image_result + j * block_information->image_information->width *
				block_information->image_information->channels + i * block_information->image_information->channels) = (u8)r32_round(last_pixel.r);
			*(block_information->image_result + j * block_information->image_information->width *
				block_information->image_information->channels + i * block_information->image_information->channels + 1) = (u8)r32_round(last_pixel.g);
			*(block_information->image_result + j * block_information->image_information->width *
				block_information->image_information->channels + i * block_information->image_information->channels + 2) = (u8)r32_round(last_pixel.b);
			*(block_information->image_result + j * block_information->image_information->width *
				block_information->image_information->channels + i * block_information->image_information->channels + 3) = 255;
		}
	}

	return 0;
}

extern void calculate_blocks_from_prologuesT(const u8* image_bytes,
	Image_Information* image_information,
	r32** prologuesT,
	s32 parallelism_level_x,
	s32 parallelism_level_y,
	const Block* image_blocks,
	s32* horizontal_domain_transforms,
	r32** rf_table,
	s32 iteration,
	s32 number_of_threads,
	u8* image_result)
{
	Thread_Block_PrologueT_Information* thread_informations = (Thread_Block_PrologueT_Information*)alloc_memory(sizeof(Thread_Block_PrologueT_Information)
		* parallelism_level_x * parallelism_level_y);

	// Fill Thread Informations
	for (s32 i = 0; i < parallelism_level_y; ++i)
	{
		r32* last_prologuesT = 0;

		for (s32 j = 0; j < parallelism_level_x; ++j)
		{
			(thread_informations + i * parallelism_level_x + j)->block = image_blocks + i * parallelism_level_x + j;
			(thread_informations + i * parallelism_level_x + j)->image_bytes = image_bytes;
			(thread_informations + i * parallelism_level_x + j)->image_information = image_information;
			(thread_informations + i * parallelism_level_x + j)->domain_transforms = horizontal_domain_transforms;
			(thread_informations + i * parallelism_level_x + j)->rf_table = rf_table;
			(thread_informations + i * parallelism_level_x + j)->iteration = iteration;
			(thread_informations + i * parallelism_level_x + j)->last_prologuesT = last_prologuesT;
			(thread_informations + i * parallelism_level_x + j)->image_result = image_result;

			last_prologuesT = *(prologuesT + i * parallelism_level_x + j);
		}
	}

	void** active_threads = (void**)alloc_memory(sizeof(void*) * number_of_threads);
	s32 num_active_threads = 0;

	// Calculate Blocks
	for (s32 i = 0; i < parallelism_level_x; ++i)
	{
		for (s32 j = 0; j < parallelism_level_y; ++j)
		{
			active_threads[num_active_threads++] = create_thread(fill_blocks_from_prologuesT_thread_proc, thread_informations + i * parallelism_level_x + j);

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

intern s32 _stdcall fill_incomplete_prologuesT_thread_proc(void* thread_information)
{
	Thread_Incomplete_PrologueT_Information* prologueT_information = (Thread_Incomplete_PrologueT_Information*)thread_information;

	for (s32 j = prologueT_information->block->y, count = 0; j < prologueT_information->block->y + prologueT_information->block->height; ++j, ++count)
	{
		R32_Color last_pixel = { 0.0f, 0.0f, 0.0f };
		R32_Color current_pixel = { 0.0f, 0.0f, 0.0f };

		*(prologueT_information->last_prologueT_contribution + count) = 1;

		for (s32 i = prologueT_information->block->x; i < prologueT_information->block->x + prologueT_information->block->width; ++i)
		{
			s32 d = *(prologueT_information->domain_transforms + j * prologueT_information->image_information->width + i);
			r32 w = (prologueT_information->rf_table)[prologueT_information->iteration][d];
			current_pixel.r = *(prologueT_information->image_bytes + j * prologueT_information->image_information->width *
				prologueT_information->image_information->channels + i * prologueT_information->image_information->channels);
			current_pixel.g = *(prologueT_information->image_bytes + j * prologueT_information->image_information->width *
				prologueT_information->image_information->channels + i * prologueT_information->image_information->channels + 1);
			current_pixel.b = *(prologueT_information->image_bytes + j * prologueT_information->image_information->width *
				prologueT_information->image_information->channels + i * prologueT_information->image_information->channels + 2);
			last_pixel.r = ((1 - w) * current_pixel.r + w * last_pixel.r);
			last_pixel.g = ((1 - w) * current_pixel.g + w * last_pixel.g);
			last_pixel.b = ((1 - w) * current_pixel.b + w * last_pixel.b);

			*(prologueT_information->last_prologueT_contribution + count) *= w;
		}

		*(prologueT_information->incomplete_prologueT + count * prologueT_information->image_information->channels + 0) = last_pixel.r;
		*(prologueT_information->incomplete_prologueT + count * prologueT_information->image_information->channels + 1) = last_pixel.g;
		*(prologueT_information->incomplete_prologueT + count * prologueT_information->image_information->channels + 2) = last_pixel.b;
		*(prologueT_information->incomplete_prologueT + count * prologueT_information->image_information->channels + 3) = 0;
	}

	return 0;
}

extern void calculate_incomplete_prologuesT(const u8* image_bytes,
	Image_Information* image_information,
	r32** prologuesT,
	r32** last_prologueT_contributions,
	s32 parallelism_level_x,
	s32 parallelism_level_y,
	const Block* image_blocks,
	s32* horizontal_domain_transforms,
	r32** rf_table,
	s32 iteration,
	s32 number_of_threads)
{
	Thread_Incomplete_PrologueT_Information* thread_informations = (Thread_Incomplete_PrologueT_Information*)alloc_memory(sizeof(Thread_Incomplete_PrologueT_Information)
		* parallelism_level_x * parallelism_level_y);

	// Fill Thread Informations
	for (s32 i = 0; i < parallelism_level_y; ++i)
		for (s32 j = 0; j < parallelism_level_x; ++j)
		{
			(thread_informations + i * parallelism_level_x + j)->block = image_blocks + i * parallelism_level_x + j;
			(thread_informations + i * parallelism_level_x + j)->image_bytes = image_bytes;
			(thread_informations + i * parallelism_level_x + j)->image_information = image_information;
			(thread_informations + i * parallelism_level_x + j)->domain_transforms = horizontal_domain_transforms;
			(thread_informations + i * parallelism_level_x + j)->rf_table = rf_table;
			(thread_informations + i * parallelism_level_x + j)->iteration = iteration;
			(thread_informations + i * parallelism_level_x + j)->last_prologueT_contribution = (r32*)alloc_memory(sizeof(r32)
				* (thread_informations + i * parallelism_level_x + j)->block->height);
			(thread_informations + i * parallelism_level_x + j)->incomplete_prologueT = (r32*)alloc_memory(sizeof(r32)
				* (thread_informations + i * parallelism_level_x + j)->block->height * image_information->channels);
		}

	void** active_threads = (void**)alloc_memory(sizeof(void*) * number_of_threads);
	s32 num_active_threads = 0;

	// Calculate Blocks
	for (s32 i = 0; i < parallelism_level_y; ++i)
	{
		for (s32 j = 0; j < parallelism_level_x; ++j)
		{
			active_threads[num_active_threads++] = create_thread(fill_incomplete_prologuesT_thread_proc, thread_informations + i * parallelism_level_x + j);

			if (num_active_threads == number_of_threads)
			{
				join_threads(num_active_threads, active_threads, 1);
				num_active_threads = 0;
			}
		}
	}

	join_threads(num_active_threads, active_threads, 1);

	// Store Prologues Results
	for (s32 i = 0; i < parallelism_level_y; ++i)
	{
		for (s32 j = 0; j < parallelism_level_x; ++j)
		{
			*(prologuesT + i * parallelism_level_x + j) = (thread_informations + i * parallelism_level_x + j)->incomplete_prologueT;
			*(last_prologueT_contributions + i * parallelism_level_x + j) = (thread_informations + i * parallelism_level_x + j)->last_prologueT_contribution;
		}
	}

	dealloc_memory(active_threads);
}