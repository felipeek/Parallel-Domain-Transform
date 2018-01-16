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

void image_rgb_to_yuv(const r32* image_bytes, s32 image_width, s32 image_height, s32 image_channels, r32* output)
{
	for (s32 i = 0; i < image_height; ++i)
		for (s32 j = 0; j < image_width; ++j)
		{
			r32 r = image_bytes[i * image_width * image_channels + j * image_channels];
			r32 g = image_bytes[i * image_width * image_channels + j * image_channels + 1];
			r32 b = image_bytes[i * image_width * image_channels + j * image_channels + 2];

			r32 y = 0.299f * r + 0.587f * g + 0.114f * b;
			r32 u = -0.14713f * r - 0.28886f * g + 0.436f * b;
			r32 v = 0.615f * r - 0.51499f * g - 0.10001f * b;

			output[i * image_width * image_channels + j * image_channels] = y;
			output[i * image_width * image_channels + j * image_channels + 1] = u;
			output[i * image_width * image_channels + j * image_channels + 2] = v;
			if (image_channels >= 4) output[i * image_width * image_channels + j * image_channels + 3] = 1.0f;
		}
}

void image_yuv_to_rgb(const r32* image_bytes, s32 image_width, s32 image_height, s32 image_channels, r32* output)
{
	for (s32 i = 0; i < image_height; ++i)
		for (s32 j = 0; j < image_width; ++j)
		{
			r32 y = image_bytes[i * image_width * image_channels + j * image_channels];
			r32 u = image_bytes[i * image_width * image_channels + j * image_channels + 1];
			r32 v = image_bytes[i * image_width * image_channels + j * image_channels + 2];
			
			r32 r = r32_clamp(1.0f * y + 0.0f * u + 1.13983f * v, 0.0f, 1.0f);
			r32 g = r32_clamp(1.0f * y - 0.39465f * u - 0.58060f * v, 0.0f, 1.0f);
			r32 b = r32_clamp(1.0f * y + 2.03211f * u + 0.0f * v, 0.0f, 1.0f);

			output[i * image_width * image_channels + j * image_channels] = r;
			output[i * image_width * image_channels + j * image_channels + 1] = g;
			output[i * image_width * image_channels + j * image_channels + 2] = b;
			if (image_channels >= 4) output[i * image_width * image_channels + j * image_channels + 3] = 1.0f;
		}
}

static Thread_Proc_Return_Type _stdcall fill_block_domain_transforms_thread_proc(void* thread_dt_information)
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
			last_pixel.r = vdt_information->image_bytes[i * vdt_information->image_information->width * vdt_information->image_information->channels +
				(vdt_information->block->x - 1) * vdt_information->image_information->channels];
			last_pixel.g = vdt_information->image_bytes[i * vdt_information->image_information->width * vdt_information->image_information->channels +
				(vdt_information->block->x - 1) * vdt_information->image_information->channels + 1];
			last_pixel.b = vdt_information->image_bytes[i * vdt_information->image_information->width * vdt_information->image_information->channels +
				(vdt_information->block->x - 1) * vdt_information->image_information->channels + 2];
		}
		else
		{
			last_pixel.r = vdt_information->image_bytes[i * vdt_information->image_information->width * vdt_information->image_information->channels +
				vdt_information->block->x * vdt_information->image_information->channels];
			last_pixel.g = vdt_information->image_bytes[i * vdt_information->image_information->width * vdt_information->image_information->channels +
				vdt_information->block->x * vdt_information->image_information->channels + 1];
			last_pixel.b = vdt_information->image_bytes[i * vdt_information->image_information->width * vdt_information->image_information->channels +
				vdt_information->block->x * vdt_information->image_information->channels + 2];
		}

		for (s32 j = vdt_information->block->x; j < vdt_information->block->x + vdt_information->block->width; ++j)
		{
			current_pixel.r = vdt_information->image_bytes[i * vdt_information->image_information->width * vdt_information->image_information->channels +
				j * vdt_information->image_information->channels];
			current_pixel.g = vdt_information->image_bytes[i * vdt_information->image_information->width * vdt_information->image_information->channels +
				j * vdt_information->image_information->channels + 1];
			current_pixel.b = vdt_information->image_bytes[i * vdt_information->image_information->width * vdt_information->image_information->channels +
				j * vdt_information->image_information->channels + 2];

			sum_channels_difference = absolute(current_pixel.r - last_pixel.r) + absolute(current_pixel.g - last_pixel.g) + absolute(current_pixel.b - last_pixel.b);

			// @NOTE: 'd' should be 1.0f + s_s / s_r * sum_diff
			// However, we will store just sum_diff.
			// 1.0f + s_s / s_r will be calculated later when calculating the RF table.
			// This is done this way because the sum_diff is perfect to be used as the index of the RF table.
			// d = 1.0f + (vdt_information->spatial_factor / vdt_information->range_factor) * sum_channels_difference;
			d = (s32)r32_round(sum_channels_difference * 255.0f);

			vdt_information->horizontal_domain_transforms[i * vdt_information->image_information->width + j] = d;

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
			last_pixel.r = vdt_information->image_bytes[(vdt_information->block->y - 1) * vdt_information->image_information->width *
				vdt_information->image_information->channels + i * vdt_information->image_information->channels];
			last_pixel.g = vdt_information->image_bytes[(vdt_information->block->y - 1) * vdt_information->image_information->width *
				vdt_information->image_information->channels + i * vdt_information->image_information->channels + 1];
			last_pixel.b = vdt_information->image_bytes[(vdt_information->block->y - 1) * vdt_information->image_information->width *
				vdt_information->image_information->channels + i * vdt_information->image_information->channels + 2];
		}
		else
		{
			last_pixel.r = vdt_information->image_bytes[vdt_information->block->y * vdt_information->image_information->width *
				vdt_information->image_information->channels + i * vdt_information->image_information->channels];
			last_pixel.g = vdt_information->image_bytes[vdt_information->block->y * vdt_information->image_information->width *
				vdt_information->image_information->channels + i * vdt_information->image_information->channels + 1];
			last_pixel.b = vdt_information->image_bytes[vdt_information->block->y * vdt_information->image_information->width *
				vdt_information->image_information->channels + i * vdt_information->image_information->channels + 2];
		}

		for (s32 j = vdt_information->block->y; j < vdt_information->block->y + vdt_information->block->height; ++j)
		{
			current_pixel.r = vdt_information->image_bytes[j * vdt_information->image_information->width * vdt_information->image_information->channels +
				i * vdt_information->image_information->channels];
			current_pixel.g = vdt_information->image_bytes[j * vdt_information->image_information->width * vdt_information->image_information->channels +
				i * vdt_information->image_information->channels + 1];
			current_pixel.b = vdt_information->image_bytes[j * vdt_information->image_information->width * vdt_information->image_information->channels +
				i * vdt_information->image_information->channels + 2];

			sum_channels_difference = absolute(current_pixel.r - last_pixel.r) + absolute(current_pixel.g - last_pixel.g) + absolute(current_pixel.b - last_pixel.b);

			// @NOTE: 'd' should be 1.0f + s_s / s_r * sum_diff
			// However, we will store just sum_diff.
			// 1.0f + s_s / s_r will be calculated later when calculating the RF table.
			// This is done this way because the sum_diff is perfect to be used as the index of the RF table.
			// d = 1.0f + (vdt_information->spatial_factor / vdt_information->range_factor) * sum_channels_difference;
			d = (s32)r32_round(sum_channels_difference * 255.0f);

			vdt_information->vertical_domain_transforms[j * vdt_information->image_information->width + i] = d;
			
			last_pixel.r = current_pixel.r;
			last_pixel.g = current_pixel.g;
			last_pixel.b = current_pixel.b;
		}
	}

	return 0;
}

static void fill_domain_transforms(s32* vertical_domain_transforms,
	s32* horizontal_domain_transforms,
	const r32* image_bytes,
	const Image_Information* image_information,
	r32 spatial_factor,
	r32 range_factor,
	s32 number_of_threads,
	s32 parallelism_level_x,
	s32 parallelism_level_y,
	const Block* image_blocks,
	Thread_DT_Information* thread_informations_memory,
	Thread_Handler* active_threads_memory)
{
	// Fill DT Informations
	for (s32 i = 0; i < parallelism_level_y; ++i)
	{
		for (s32 j = 0; j < parallelism_level_x; ++j)
		{
			thread_informations_memory[i * parallelism_level_x + j].block = image_blocks + i * parallelism_level_x + j;
			thread_informations_memory[i * parallelism_level_x + j].image_bytes = image_bytes;
			thread_informations_memory[i * parallelism_level_x + j].image_information = image_information;
			thread_informations_memory[i * parallelism_level_x + j].vertical_domain_transforms = vertical_domain_transforms;
			thread_informations_memory[i * parallelism_level_x + j].horizontal_domain_transforms = horizontal_domain_transforms;
		}
	}

	s32 num_active_threads = 0;

	// Fill Domain Transforms
	for (s32 i = 0; i < parallelism_level_y; ++i)
	{
		for (s32 j = 0; j < parallelism_level_x; ++j)
		{
			active_threads_memory[num_active_threads++] = create_thread(fill_block_domain_transforms_thread_proc, thread_informations_memory + i * parallelism_level_x + j);
			if (num_active_threads == number_of_threads)
			{
				join_threads(num_active_threads, active_threads_memory, 1);
				num_active_threads = 0;
			}
		}
	}

	join_threads(num_active_threads, active_threads_memory, 1);
}

/*static void paint_block(r32* img_data,
	Image_Information* img,
	const Block* b,
	const R32_Color* c)
{
	for (s32 i = b->y; i < b->y + b->height; ++i)
	{
		for (s32 j = b->x; j < b->x + b->width; ++j)
		{
			img_data[i * img->width * img->channels + j * img->channels] = c->r;
			img_data[i * img->width * img->channels + j * img->channels + 1] = c->g;
			img_data[i * img->width * img->channels + j * img->channels + 2] = c->b;
		}
	}
}*/

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
	r32* image_result)
{
	//memcpy(image_result, image_bytes, image_width * image_height * image_channels * sizeof(r32));

	/* CREATE BLOCKS */
	Block* image_blocks = (Block*)alloc_memory(sizeof(Block) * parallelism_level_x * parallelism_level_y);

	Image_Information image_information;

	image_information.height = image_height;
	image_information.width = image_width;
	image_information.channels = image_channels;

	// Separate image in blocks.
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
			image_blocks[i * parallelism_level_x + j] = b;

			blocks_accumulate_x = blocks_accumulate_x + current_block_width;
		}

		blocks_accumulate_y = blocks_accumulate_y + current_block_height;
		blocks_accumulate_x = 0;
	}

	/* PRE-ALLOC MEMORY */

	r32* rf_coefficients = (r32*)alloc_memory(sizeof(r32) * num_iterations);
	r32** rf_table = (r32**)alloc_memory(sizeof(r32*) * num_iterations);
	for (s32 i = 0; i < num_iterations; ++i)
		rf_table[i] = (r32*)alloc_memory(sizeof(r32) * RF_TABLE_SIZE);
	s32* vertical_domain_transforms = (s32*)alloc_memory(sizeof(s32) * image_height * image_width);
	s32* horizontal_domain_transforms = (s32*)alloc_memory(sizeof(s32) * image_height * image_width);
	Thread_Handler* active_threads_memory = (Thread_Handler*)alloc_memory(sizeof(Thread_Handler) * number_of_threads);
	Thread_DT_Information* threads_informations_memory = (Thread_DT_Information*)alloc_memory(sizeof(Thread_DT_Information) * parallelism_level_x * parallelism_level_y);
	Thread_Incomplete_Prologue_Information* incomplete_prologue_thread_informations = (Thread_Incomplete_Prologue_Information*)alloc_memory(sizeof(Thread_Incomplete_Prologue_Information)
		* parallelism_level_x * parallelism_level_y);
	Thread_Block_Prologue_Information* block_prologue_thread_informations = (Thread_Block_Prologue_Information*)alloc_memory(sizeof(Thread_Block_Prologue_Information)
		* parallelism_level_x * parallelism_level_y);
	Thread_Block_Epilogue_Information* block_epilogue_thread_informations = (Thread_Block_Epilogue_Information*)alloc_memory(sizeof(Thread_Block_Epilogue_Information)
		* parallelism_level_x * parallelism_level_y);
	Thread_Incomplete_Epilogue_Information* incomplete_epilogue_thread_informations = (Thread_Incomplete_Epilogue_Information*)alloc_memory(sizeof(Thread_Incomplete_Epilogue_Information)
		* parallelism_level_x * parallelism_level_y);
	Thread_Incomplete_PrologueT_Information* incomplete_prologueT_thread_informations = (Thread_Incomplete_PrologueT_Information*)alloc_memory(sizeof(Thread_Incomplete_PrologueT_Information)
		* parallelism_level_x * parallelism_level_y);
	Thread_Block_PrologueT_Information* block_prologueT_thread_informations = (Thread_Block_PrologueT_Information*)alloc_memory(sizeof(Thread_Block_PrologueT_Information)
		* parallelism_level_x * parallelism_level_y);
	Thread_Block_EpilogueT_Information* block_epilogueT_thread_informations = (Thread_Block_EpilogueT_Information*)alloc_memory(sizeof(Thread_Block_EpilogueT_Information)
		* parallelism_level_x * parallelism_level_y);
	Thread_Incomplete_EpilogueT_Information* incomplete_epilogueT_thread_informations = (Thread_Incomplete_EpilogueT_Information*)alloc_memory(sizeof(Thread_Incomplete_EpilogueT_Information)
		* parallelism_level_x * parallelism_level_y);
	r32** prologues = (r32**)alloc_memory(sizeof(r32*) * parallelism_level_x * parallelism_level_y);
	r32** last_prologue_contributions = (r32**)alloc_memory(sizeof(r32*) * parallelism_level_x * parallelism_level_y);
	r32** epilogues = (r32**)alloc_memory(sizeof(r32*) * parallelism_level_x * parallelism_level_y);
	r32** last_epilogue_contributions = (r32**)alloc_memory(sizeof(r32*) * parallelism_level_x * parallelism_level_y);
	r32** prologuesT = (r32**)alloc_memory(sizeof(r32*) * parallelism_level_x * parallelism_level_y);
	r32** last_prologueT_contributions = (r32**)alloc_memory(sizeof(r32*) * parallelism_level_x * parallelism_level_y);
	r32** epiloguesT = (r32**)alloc_memory(sizeof(r32*) * parallelism_level_x * parallelism_level_y);
	r32** last_epilogueT_contributions = (r32**)alloc_memory(sizeof(r32*) * parallelism_level_x * parallelism_level_y);
	r32* mask_result = (r32*)alloc_memory(sizeof(r32) * image_width * image_height * image_channels);	// auxiliary memory to store mask result

	for (s32 i = 0; i < parallelism_level_y; ++i)
		for (s32 j = 0; j < parallelism_level_x; ++j)
		{
			prologuesT[i * parallelism_level_x + j] = (r32*)alloc_memory(sizeof(r32)
				* image_blocks[i * parallelism_level_x + j].height * image_channels);
			last_prologueT_contributions[i * parallelism_level_x + j] = (r32*)alloc_memory(sizeof(r32)
				* image_blocks[i * parallelism_level_x + j].height * image_channels);
		}

	for (s32 i = 0; i < parallelism_level_y; ++i)
		for (s32 j = 0; j < parallelism_level_x; ++j)
		{
			epiloguesT[i * parallelism_level_x + j] = (r32*)alloc_memory(sizeof(r32)
				* image_blocks[i * parallelism_level_x + j].height * image_channels);
			last_epilogueT_contributions[i * parallelism_level_x + j] = (r32*)alloc_memory(sizeof(r32)
				* image_blocks[i * parallelism_level_x + j].height * image_channels);
		}

	for (s32 i = 0; i < parallelism_level_y; ++i)
		for (s32 j = 0; j < parallelism_level_x; ++j)
		{
			prologues[i * parallelism_level_x + j] = (r32*)alloc_memory(sizeof(r32)
				* image_blocks[i * parallelism_level_x + j].width * image_channels);
			last_prologue_contributions[i * parallelism_level_x + j] = (r32*)alloc_memory(sizeof(r32)
				* image_blocks[i * parallelism_level_x + j].width * image_channels);
		}

	for (s32 i = 0; i < parallelism_level_y; ++i)
		for (s32 j = 0; j < parallelism_level_x; ++j)
		{
			epilogues[i * parallelism_level_x + j] = (r32*)alloc_memory(sizeof(r32)
				* image_blocks[i * parallelism_level_x + j].width * image_channels);
			last_epilogue_contributions[i * parallelism_level_x + j] = (r32*)alloc_memory(sizeof(r32)
				* image_blocks[i * parallelism_level_x + j].width * image_channels);
		}

	/* ************************ */

	// Pre-calculate RF feedback coefficients
	for (s32 i = 0; i < num_iterations; ++i)
	{
		// calculating RF feedback coefficient 'a' from desired variance
		// 'a' will change each iteration while the domain transform will remain constant
		r32 current_standard_deviation = spatial_factor * SQRT3 * (r32_pow(2.0f, (r32)(num_iterations - (i + 1))) / r32_sqrt(r32_pow(4.0f, (r32)num_iterations) - 1));
		r32 a = r32_exp(-SQRT2 / current_standard_deviation);

		rf_coefficients[i] = a;
	}

	// Pre-calculate RF table
	r32 d;

	for (s32 i = 0; i < num_iterations; ++i)
	{
		for (s32 j = 0; j < RF_TABLE_SIZE; ++j)
		{
			d = 1.0f + (spatial_factor / range_factor) * ((r32)j / 255.0f);
			rf_table[i][j] = r32_pow(rf_coefficients[i], d);
		}
	}

	// Pre-calculate Domain Transform
	fill_domain_transforms(vertical_domain_transforms,
		horizontal_domain_transforms,
		image_bytes,
		&image_information,
		spatial_factor,
		range_factor,
		number_of_threads,
		parallelism_level_x,
		parallelism_level_y,
		image_blocks,
		threads_informations_memory,
		active_threads_memory);

	// Filter Iterations (for scribbles)
	for (s32 i = 0; i < num_iterations; ++i)
	{
		// Calculate Incomplete Prologues
		// if first iteration, send scribbles_bytes.
		if (i == 0)
			calculate_incomplete_prologues(scribbles_bytes,
				&image_information,
				prologues,
				last_prologue_contributions,
				parallelism_level_x,
				parallelism_level_y,
				image_blocks,
				vertical_domain_transforms,
				rf_table,
				i,
				number_of_threads,
				incomplete_prologue_thread_informations,
				active_threads_memory);
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
				number_of_threads,
				incomplete_prologue_thread_informations,
				active_threads_memory);

		// Calculate Complete Prologues
		calculate_complete_prologues(&image_information,
			prologues,
			prologues,
			last_prologue_contributions,
			image_blocks,
			parallelism_level_x,
			parallelism_level_y);

		// Calculate Full Blocks From Prologues
		// if first iteration, send scribbles_bytes.
		if (i == 0)
			calculate_blocks_from_prologues(scribbles_bytes,
				&image_information,
				prologues,
				parallelism_level_x,
				parallelism_level_y,
				image_blocks,
				vertical_domain_transforms,
				rf_table,
				i,
				number_of_threads,
				image_result,
				block_prologue_thread_informations,
				active_threads_memory);
		else
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
				image_result,
				block_prologue_thread_informations,
				active_threads_memory);

		// Calculate Incomplete Epilogues
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
			number_of_threads,
			incomplete_epilogue_thread_informations,
			active_threads_memory);

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
			image_result,
			block_epilogue_thread_informations,
			active_threads_memory);

		// Calculate Incomplete ProloguesT
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
			number_of_threads,
			incomplete_prologueT_thread_informations,
			active_threads_memory);

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
			image_result,
			block_prologueT_thread_informations,
			active_threads_memory);

		// Calculate Incomplete EpiloguesT
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
			number_of_threads,
			incomplete_epilogueT_thread_informations,
			active_threads_memory);

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
			image_result,
			block_epilogueT_thread_informations,
			active_threads_memory);
	}

	// Filter Iterations (for mask)
	for (s32 i = 0; i < num_iterations; ++i)
	{
		// Calculate Incomplete Prologues
		// if first iteration, send mask_bytes.
		if (i == 0)
			calculate_incomplete_prologues(mask_bytes,
				&image_information,
				prologues,
				last_prologue_contributions,
				parallelism_level_x,
				parallelism_level_y,
				image_blocks,
				vertical_domain_transforms,
				rf_table,
				i,
				number_of_threads,
				incomplete_prologue_thread_informations,
				active_threads_memory);
		else
			calculate_incomplete_prologues(mask_result,
				&image_information,
				prologues,
				last_prologue_contributions,
				parallelism_level_x,
				parallelism_level_y,
				image_blocks,
				vertical_domain_transforms,
				rf_table,
				i,
				number_of_threads,
				incomplete_prologue_thread_informations,
				active_threads_memory);

		// Calculate Complete Prologues
		calculate_complete_prologues(&image_information,
			prologues,
			prologues,
			last_prologue_contributions,
			image_blocks,
			parallelism_level_x,
			parallelism_level_y);

		// Calculate Full Blocks From Prologues
		// if first iteration, send mask_bytes.
		if (i == 0)
			calculate_blocks_from_prologues(mask_bytes,
				&image_information,
				prologues,
				parallelism_level_x,
				parallelism_level_y,
				image_blocks,
				vertical_domain_transforms,
				rf_table,
				i,
				number_of_threads,
				mask_result,
				block_prologue_thread_informations,
				active_threads_memory);
		else
			calculate_blocks_from_prologues(mask_result,
				&image_information,
				prologues,
				parallelism_level_x,
				parallelism_level_y,
				image_blocks,
				vertical_domain_transforms,
				rf_table,
				i,
				number_of_threads,
				mask_result,
				block_prologue_thread_informations,
				active_threads_memory);

		// Calculate Incomplete Epilogues
		calculate_incomplete_epilogues(mask_result,
			&image_information,
			epilogues,
			last_epilogue_contributions,
			parallelism_level_x,
			parallelism_level_y,
			image_blocks,
			vertical_domain_transforms,
			rf_table,
			i,
			number_of_threads,
			incomplete_epilogue_thread_informations,
			active_threads_memory);

		// Calculate Complete Epilogues
		calculate_complete_epilogues(&image_information,
			epilogues,
			epilogues,
			last_epilogue_contributions,
			image_blocks,
			parallelism_level_x,
			parallelism_level_y);

		// Calculate Full Blocks From Epilogues
		calculate_blocks_from_epilogues(mask_result,
			&image_information,
			epilogues,
			parallelism_level_x,
			parallelism_level_y,
			image_blocks,
			vertical_domain_transforms,
			rf_table,
			i,
			number_of_threads,
			mask_result,
			block_epilogue_thread_informations,
			active_threads_memory);

		// Calculate Incomplete ProloguesT
		calculate_incomplete_prologuesT(mask_result,
			&image_information,
			prologuesT,
			last_prologueT_contributions,
			parallelism_level_x,
			parallelism_level_y,
			image_blocks,
			horizontal_domain_transforms,
			rf_table,
			i,
			number_of_threads,
			incomplete_prologueT_thread_informations,
			active_threads_memory);

		// Calculate Complete ProloguesT
		calculate_complete_prologuesT(&image_information,
			prologuesT,
			prologuesT,
			last_prologueT_contributions,
			image_blocks,
			parallelism_level_x,
			parallelism_level_y);

		// Calculate Full Blocks From ProloguesT
		calculate_blocks_from_prologuesT(mask_result,
			&image_information,
			prologuesT,
			parallelism_level_x,
			parallelism_level_y,
			image_blocks,
			horizontal_domain_transforms,
			rf_table,
			i,
			number_of_threads,
			mask_result,
			block_prologueT_thread_informations,
			active_threads_memory);

		// Calculate Incomplete EpiloguesT
		calculate_incomplete_epiloguesT(mask_result,
			&image_information,
			epiloguesT,
			last_epilogueT_contributions,
			parallelism_level_x,
			parallelism_level_y,
			image_blocks,
			horizontal_domain_transforms,
			rf_table,
			i,
			number_of_threads,
			incomplete_epilogueT_thread_informations,
			active_threads_memory);

		// Calculate Complete EpiloguesT
		calculate_complete_epiloguesT(&image_information,
			epiloguesT,
			epiloguesT,
			last_epilogueT_contributions,
			image_blocks,
			parallelism_level_x,
			parallelism_level_y);

		// Calculate Full Blocks From EpiloguesT
		calculate_blocks_from_epiloguesT(mask_result,
			&image_information,
			epiloguesT,
			parallelism_level_x,
			parallelism_level_y,
			image_blocks,
			horizontal_domain_transforms,
			rf_table,
			i,
			number_of_threads,
			mask_result,
			block_epilogueT_thread_informations,
			active_threads_memory);
	}

	// Scribble Result and Mask Result are ready!
	for (s32 i = 0; i < image_height; ++i)
		for (s32 j = 0; j < image_width; ++j)
		{
			if (mask_result[i * image_width * image_channels + j * image_channels] == 0.0f)
			{
				image_result[i * image_width * image_channels + j * image_channels] = 0.0f;
				image_result[i * image_width * image_channels + j * image_channels + 1] = 0.0f;
				image_result[i * image_width * image_channels + j * image_channels + 2] = 0.0f;
			}
			else
			{
				image_result[i * image_width * image_channels + j * image_channels] = image_result[i * image_width * image_channels + j * image_channels] /
					mask_result[i * image_width * image_channels + j * image_channels];

				image_result[i * image_width * image_channels + j * image_channels + 1] = image_result[i * image_width * image_channels + j * image_channels + 1] /
					mask_result[i * image_width * image_channels + j * image_channels + 1];

				image_result[i * image_width * image_channels + j * image_channels + 2] = image_result[i * image_width * image_channels + j * image_channels + 2] /
					mask_result[i * image_width * image_channels + j * image_channels + 2];
			}
		}

	// YUV Conversion
	image_rgb_to_yuv(image_result, image_width, image_height, image_channels, image_result);
	image_rgb_to_yuv(image_bytes, image_width, image_height, image_channels, mask_result);	// Uses mask_result to store memory.

	// Replace Y channel
	for (s32 i = 0; i < image_height; ++i)
		for (s32 j = 0; j < image_width; ++j)
		{
			image_result[i * image_width * image_channels + j * image_channels] = mask_result[i * image_width * image_channels + j * image_channels];
		}

	// RGB Conversion
	image_yuv_to_rgb(image_result, image_width, image_height, image_channels, image_result);


	// Dealloc memory
	for (s32 i = 0; i < parallelism_level_y; ++i)
		for (s32 j = 0; j < parallelism_level_x; ++j)
			dealloc_memory(prologues[i * parallelism_level_x + j]);
	dealloc_memory(prologues);

	for (s32 i = 0; i < parallelism_level_y; ++i)
		for (s32 j = 0; j < parallelism_level_x; ++j)
			dealloc_memory(last_prologue_contributions[i * parallelism_level_x + j]);
	dealloc_memory(last_prologue_contributions);

	// Dealloc memory
	for (s32 i = 0; i < parallelism_level_y; ++i)
		for (s32 j = 0; j < parallelism_level_x; ++j)
			dealloc_memory(epilogues[i * parallelism_level_x + j]);
	dealloc_memory(epilogues);

	for (s32 i = 0; i < parallelism_level_y; ++i)
		for (s32 j = 0; j < parallelism_level_x; ++j)
			dealloc_memory(last_epilogue_contributions[i * parallelism_level_x + j]);
	dealloc_memory(last_epilogue_contributions);

	// Dealloc memory
	for (s32 i = 0; i < parallelism_level_y; ++i)
		for (s32 j = 0; j < parallelism_level_x; ++j)
			dealloc_memory(prologuesT[i * parallelism_level_x + j]);
	dealloc_memory(prologuesT);

	for (s32 i = 0; i < parallelism_level_y; ++i)
		for (s32 j = 0; j < parallelism_level_x; ++j)
			dealloc_memory(last_prologueT_contributions[i * parallelism_level_x + j]);
	dealloc_memory(last_prologueT_contributions);

	// Dealloc memory
	for (s32 i = 0; i < parallelism_level_y; ++i)
		for (s32 j = 0; j < parallelism_level_x; ++j)
			dealloc_memory(epiloguesT[i * parallelism_level_x + j]);
	dealloc_memory(epiloguesT);

	for (s32 i = 0; i < parallelism_level_y; ++i)
		for (s32 j = 0; j < parallelism_level_x; ++j)
			dealloc_memory(last_epilogueT_contributions[i * parallelism_level_x + j]);
	dealloc_memory(last_epilogueT_contributions);

	// Dealloc memory
	for (s32 i = 0; i < num_iterations; ++i)
		dealloc_memory(rf_table[i]);
	dealloc_memory(rf_table);

	dealloc_memory(image_blocks);
	dealloc_memory(vertical_domain_transforms);
	dealloc_memory(horizontal_domain_transforms);
	
	dealloc_memory(mask_result);


//	dealloc_arena_memory();

	return 0;

	return 0;
}