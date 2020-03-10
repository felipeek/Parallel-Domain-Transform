#define _CRT_SECURE_NO_WARNINGS
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <stb_image.h>
#include <stb_image_write.h>
#include "Util.h"
#include "ParallelDomainTransform.h"

#define DEFAULT_IMAGE_PATH "./res/img.jpg"
#define DEFAULT_SPATIAL_FACTOR 0.1f
#define DEFAULT_RANGE_FACTOR 1.0f
#define DEFAULT_NUM_ITERATIONS 4
#define DEFAULT_PARALLELISM_LEVEL_X 16
#define DEFAULT_PARALLELISM_LEVEL_Y 16
#define DEFAULT_NUMBER_OF_THREADS 1
#define DEFAULT_COLLECT_TIME 1
#define DEFAULT_RESULT_PATH "./res/output.png"
#define DEFAULT_IMAGE_CHANNELS 4

static r32* load_image(const s8* image_path,
	s32* image_width,
	s32* image_height,
	s32* image_channels,
	s32 desired_channels)
{
	u8* auxiliary_data = stbi_load(image_path, image_width, image_height, image_channels, DEFAULT_IMAGE_CHANNELS);

	if (auxiliary_data == 0)
		return 0;

	// @TODO: check WHY this is happening
	*image_channels = 4;

	r32* image_data = (r32*)alloc_memory(sizeof(r32) * *image_height * *image_width * *image_channels);

	for (s32 i = 0; i < *image_height; ++i)
	{
		for (s32 j = 0; j < *image_width; ++j)
		{
			image_data[i * *image_width * *image_channels + j * *image_channels] =
				auxiliary_data[i * *image_width * *image_channels + j * *image_channels] / 255.0f;
			image_data[i * *image_width * *image_channels + j * *image_channels + 1] =
				auxiliary_data[i * *image_width * *image_channels + j * *image_channels + 1] / 255.0f;
			image_data[i * *image_width * *image_channels + j * *image_channels + 2] =
				auxiliary_data[i * *image_width * *image_channels + j * *image_channels + 2] / 255.0f;
			image_data[i * *image_width * *image_channels + j * *image_channels + 3] = 1.0f;
		}
	}

	stbi_image_free(auxiliary_data);

	return image_data;
}

static void store_image(const s8* result_path,
	s32 image_width,
	s32 image_height,
	s32 image_channels,
	const r32* image_bytes)
{
	u8* auxiliary_data = (u8*)alloc_memory(image_height * image_width * sizeof(u8) * image_channels);

	for (s32 i = 0; i < image_height; ++i)
	{
		for (s32 j = 0; j < image_width; ++j)
		{
			auxiliary_data[i * image_width * image_channels + j * image_channels] = (u8)r32_round(255.0f * image_bytes[i * image_width * image_channels + j * image_channels]);
			auxiliary_data[i * image_width * image_channels + j * image_channels + 1] = (u8)r32_round(255.0f * image_bytes[i * image_width * image_channels + j * image_channels + 1]);
			auxiliary_data[i * image_width * image_channels + j * image_channels + 2] = (u8)r32_round(255.0f * image_bytes[i * image_width * image_channels + j * image_channels + 2]);
			auxiliary_data[i * image_width * image_channels + j * image_channels + 3] = 255;
		}
	}
	
	stbi_write_png(result_path, image_width, image_height, image_channels, auxiliary_data, image_width * image_channels);
	dealloc_memory(auxiliary_data);
}

static void print_help_message(const s8* exe_name)
{
	print("Usage: %s\n\n", exe_name);
	print("Optional Parameters:\n");
	print("\tImage Path: -p <string>\n");
	print("\tSpatial Factor: -s <r32>\n");
	print("\tRange Factor: -r <r32>\n");
	print("\tNumber of Iterations: -i <s32>\n");
	print("\tParallelism Level (X Axis): -x <s32>\n");
	print("\tParallelism Level (Y Axis): -y <s32>\n");
	print("\tNumber of Threads (MAX 64): -t <s32>\n");
	print("\tCollect Time: -c <1 or 0>\n");
	print("\tResult Path: -o <string>\n");
	print("\tHelp: -h\n");
}

extern s32 main(s32 argc, s8** argv)
{
	s8 default_image_path[] = DEFAULT_IMAGE_PATH;
	s8 default_result_path[] = DEFAULT_RESULT_PATH;
	
	s8* image_path = 0;
	r32 spatial_factor = DEFAULT_SPATIAL_FACTOR;
	r32 range_factor = DEFAULT_RANGE_FACTOR;
	s32 num_iterations = DEFAULT_NUM_ITERATIONS;
	s32 parallelism_level_x = DEFAULT_PARALLELISM_LEVEL_X;
	s32 parallelism_level_y = DEFAULT_PARALLELISM_LEVEL_Y;
	s32 number_of_threads = DEFAULT_NUMBER_OF_THREADS;
	s32 collect_time = DEFAULT_COLLECT_TIME;
	s8* result_path = 0;
	
	if (argc % 2 != 0 && argc > 1)
	{
		for (s32 i = 1; i < argc; i += 2)
		{
			if ((argv[i][0] == '-' || argv[i][0] == '/') && str_length(argv[i]) == 2)
			{
				switch (argv[i][1])
				{
				// Image Path
				case 'p': {
					image_path = argv[i + 1];
				} break;
				// Spatial Factor
				case 's': {
					spatial_factor = str_to_r32(argv[i + 1]);
				} break;
				// Range Factor
				case 'r': {
					range_factor = str_to_r32(argv[i + 1]);
				} break;
				// Number of Iterations
				case 'i': {
					num_iterations = str_to_s32(argv[i + 1]);
				} break;
				// Parallelism Level X
				case 'x': {
					parallelism_level_x = str_to_s32(argv[i + 1]);
				} break;
				// Parallelism Level Y
				case 'y': {
					parallelism_level_y = str_to_s32(argv[i + 1]);
				} break;
				// Number of Threads
				case 't': {
					number_of_threads = str_to_s32(argv[i + 1]);
					if (number_of_threads > 64)
					{
						print("Warning: The maximum number of threads is 64.\n");
						print("The number of threads was automatically modified to 64.\n");
						number_of_threads = 64;
					}
				} break;
				// Collect Time
				case 'c': {
					collect_time = str_to_s32(argv[i + 1]);
				} break;
				// Result Path
				case 'o': {
					result_path = argv[i + 1];
				} break;
				// Help
				case 'h':
				default: {
					print_help_message(argv[0]);
					return 0;
				} break;
				}
			}
		}
	}
	else
	{
		print_help_message(argv[0]);
		return 0;
	}

	s32 image_width, image_height, image_channels;
	r32* image_bytes;
	r32* image_result;

	if (image_path)
		image_bytes = load_image(image_path, &image_width, &image_height, &image_channels, DEFAULT_IMAGE_CHANNELS);
	else
		image_bytes = load_image(default_image_path, &image_width, &image_height, &image_channels, DEFAULT_IMAGE_CHANNELS);

	if (!image_bytes)
	{
		print("Error: Could not load image %s\n", image_path);
		return -1;
	}

	image_result = (r32*)alloc_memory(sizeof(r32) * image_width * image_height * image_channels);

	start_clock();

	s32 domain_transform_error_code = parallel_domain_transform(image_bytes, image_width, image_height, image_channels, spatial_factor, range_factor,
		num_iterations, number_of_threads, parallelism_level_x, parallelism_level_y, image_result);

	r32 total_time = end_clock();

	switch (domain_transform_error_code)
	{
	case -1: {
		print("Error: Domain Transform Failed (Unknown Reason).\n");
		return -1;
	} break;
	}

	if (collect_time)
		print("Total time: %.3f seconds\n", total_time);

	if (result_path)
		store_image(result_path, image_width, image_height, image_channels, image_result);
	else
		store_image(default_result_path, image_width, image_height, image_channels, image_result);
	
	dealloc_memory(image_bytes);
	dealloc_memory(image_result);

	return 0;
}
