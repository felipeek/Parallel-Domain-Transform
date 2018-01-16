#include "DomainTransform.h"
#include "Util.h"

// This should be 768, since we have 2^8 possible options for each channels.
// Since domain transform is given by:
// 1 + (s_s / s_r) * (diff_channel_R + diff_channel_G + diff_channel_B)
// We will have 3 * 2^8 different possibilities for the transform of each pixel.
// 3 * 2^8 = 768
#define RF_TABLE_SIZE 768
#define SQRT3 1.7320508075f
#define SQRT2 1.4142135623f

static void fill_domain_transforms(s32* vertical_domain_transforms,
	s32* horizontal_domain_transforms,
	const r32* image_bytes,
	s32 image_width,
	s32 image_height,
	s32 image_channels,
	r32 spatial_factor,
	r32 range_factor)
{
	r32 last_pixel_r, last_pixel_g, last_pixel_b;
	r32 current_pixel_r, current_pixel_g, current_pixel_b;
	r32 sum_channels_difference;
	s32 d;

	// Fill Horizontal Domain Transforms
	for (s32 i = 0; i < image_height; ++i)
	{
		last_pixel_r = image_bytes[i * image_width * image_channels];
		last_pixel_g = image_bytes[i * image_width * image_channels + 1];
		last_pixel_b = image_bytes[i * image_width * image_channels + 2];

		for (s32 j = 0; j < image_width; ++j)
		{
			current_pixel_r = image_bytes[i * image_width * image_channels + j * image_channels];
			current_pixel_g = image_bytes[i * image_width * image_channels + j * image_channels + 1];
			current_pixel_b = image_bytes[i * image_width * image_channels + j * image_channels + 2];

			sum_channels_difference = absolute(current_pixel_r - last_pixel_r) + absolute(current_pixel_g - last_pixel_g) + absolute(current_pixel_b - last_pixel_b);

			// @NOTE: 'd' should be 1.0f + s_s / s_r * sum_diff
			// However, we will store just sum_diff.
			// 1.0f + s_s / s_r will be calculated later when calculating the RF table.
			// This is done this way because the sum_diff is perfect to be used as the index of the RF table.
			// d = 1.0f + (vdt_information->spatial_factor / vdt_information->range_factor) * sum_channels_difference;
			d = (s32)r32_round(sum_channels_difference * 255.0f);

			horizontal_domain_transforms[i * image_width + j] = d;

			last_pixel_r = current_pixel_r;
			last_pixel_g = current_pixel_g;
			last_pixel_b = current_pixel_b;
		}
	}

	// Fill Vertical Domain Transforms
	for (s32 i = 0; i < image_width; ++i)
	{
		last_pixel_r = image_bytes[i * image_channels];
		last_pixel_g = image_bytes[i * image_channels + 1];
		last_pixel_b = image_bytes[i * image_channels + 2];

		for (s32 j = 0; j < image_height; ++j)
		{
			current_pixel_r = image_bytes[j * image_width * image_channels + i * image_channels];
			current_pixel_g = image_bytes[j * image_width * image_channels + i * image_channels + 1];
			current_pixel_b = image_bytes[j * image_width * image_channels + i * image_channels + 2];

			sum_channels_difference = absolute(current_pixel_r - last_pixel_r) + absolute(current_pixel_g - last_pixel_g) + absolute(current_pixel_b - last_pixel_b);

			// @NOTE: 'd' should be 1.0f + s_s / s_r * sum_diff
			// However, we will store just sum_diff.
			// 1.0f + s_s / s_r will be calculated later when calculating the RF table.
			// This is done this way because the sum_diff is perfect to be used as the index of the RF table.
			// d = 1.0f + (vdt_information->spatial_factor / vdt_information->range_factor) * sum_channels_difference;
			d = (s32)r32_round(sum_channels_difference * 255.0f);

			vertical_domain_transforms[j * image_width + i] = d;

			last_pixel_r = current_pixel_r;
			last_pixel_g = current_pixel_g;
			last_pixel_b = current_pixel_b;
		}
	}
}

extern s32 domain_transform(const r32* image_bytes,
	s32 image_width,
	s32 image_height,
	s32 image_channels,
	r32 spatial_factor,
	r32 range_factor,
	s32 num_iterations,
	r32* image_result)
{
	/* PRE-ALLOC MEMORY */

	r32* rf_coefficients = (r32*)alloc_memory(sizeof(r32) * num_iterations);
	r32** rf_table = (r32**)alloc_memory(sizeof(r32*) * num_iterations);
	for (s32 i = 0; i < num_iterations; ++i)
		rf_table[i] = (r32*)alloc_memory(sizeof(r32) * RF_TABLE_SIZE);
	s32* vertical_domain_transforms = (s32*)alloc_memory(sizeof(s32) * image_height * image_width);
	s32* horizontal_domain_transforms = (s32*)alloc_memory(sizeof(s32) * image_height * image_width);

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
			d = 1.0f + (spatial_factor / range_factor) * ((float)j / 255.0f);
			rf_table[i][j] = r32_pow(rf_coefficients[i], d);
		}
	}

	// Pre-calculate Domain Transform
	fill_domain_transforms(vertical_domain_transforms,
		horizontal_domain_transforms,
		image_bytes,
		image_width,
		image_height,
		image_channels,
		spatial_factor,
		range_factor);

	copy_memory(image_result, image_bytes, image_width * image_channels * image_height * sizeof(r32));

	// Filter Iterations
	for (s32 n = 0; n < num_iterations; ++n)
	{
		// Horizontal Filter (Left-Right)
		for (s32 j = 0; j < image_height; ++j)
		{
			r32 last_pixel_r, last_pixel_g, last_pixel_b;
			r32 current_pixel_r, current_pixel_g, current_pixel_b;

			last_pixel_r = image_result[j * image_width * image_channels];
			last_pixel_g = image_result[j * image_width * image_channels + 1];
			last_pixel_b = image_result[j * image_width * image_channels + 2];

			for (s32 i = 0; i < image_width; ++i)
			{
				s32 d = horizontal_domain_transforms[j * image_width + i];
				r32 w = rf_table[n][d];

				current_pixel_r = image_result[j * image_width * image_channels + i * image_channels];
				current_pixel_g = image_result[j * image_width * image_channels + i * image_channels + 1];
				current_pixel_b = image_result[j * image_width * image_channels + i * image_channels + 2];

				last_pixel_r = ((1 - w) * current_pixel_r + w * last_pixel_r);
				last_pixel_g = ((1 - w) * current_pixel_g + w * last_pixel_g);
				last_pixel_b = ((1 - w) * current_pixel_b + w * last_pixel_b);

				image_result[j * image_width * image_channels + i * image_channels] = last_pixel_r;
				image_result[j * image_width * image_channels + i * image_channels + 1] = last_pixel_g;
				image_result[j * image_width * image_channels + i * image_channels + 2] = last_pixel_b;
				image_result[j * image_width * image_channels + i * image_channels + 3] = 1.0f;
			}
		}

		// Horizontal Filter (Right-Left)
		for (s32 j = 0; j < image_height; ++j)
		{
			r32 last_pixel_r, last_pixel_g, last_pixel_b;
			r32 current_pixel_r, current_pixel_g, current_pixel_b;

			last_pixel_r = image_result[j * image_width * image_channels + (image_width - 1) * image_channels];
			last_pixel_g = image_result[j * image_width * image_channels + (image_width - 1) * image_channels + 1];
			last_pixel_b = image_result[j * image_width * image_channels + (image_width - 1) * image_channels + 2];
			
			for (s32 i = image_width - 1; i >= 0; --i)
			{
				s32 d_x_position = (i < image_width - 1) ? i + 1 : i;
				s32 d = horizontal_domain_transforms[j * image_width + d_x_position];
				r32 w = rf_table[n][d];

				current_pixel_r = image_result[j * image_width * image_channels + i * image_channels];
				current_pixel_g = image_result[j * image_width * image_channels + i * image_channels + 1];
				current_pixel_b = image_result[j * image_width * image_channels + i * image_channels + 2];

				last_pixel_r = ((1 - w) * current_pixel_r + w * last_pixel_r);
				last_pixel_g = ((1 - w) * current_pixel_g + w * last_pixel_g);
				last_pixel_b = ((1 - w) * current_pixel_b + w * last_pixel_b);

				image_result[j * image_width * image_channels + i * image_channels] = last_pixel_r;
				image_result[j * image_width * image_channels + i * image_channels + 1] = last_pixel_g;
				image_result[j * image_width * image_channels + i * image_channels + 2] = last_pixel_b;
				image_result[j * image_width * image_channels + i * image_channels + 3] = 1.0f;
			}
		}

		// Vertical Filter (Top-Down)
		for (s32 i = 0; i < image_width; ++i)
		{
			r32 last_pixel_r, last_pixel_g, last_pixel_b;
			r32 current_pixel_r, current_pixel_g, current_pixel_b;

			last_pixel_r = image_result[i * image_channels];
			last_pixel_g = image_result[i * image_channels + 1];
			last_pixel_b = image_result[i * image_channels + 2];

			for (s32 j = 0; j < image_height; ++j)
			{
				s32 d = vertical_domain_transforms[j * image_width + i];
				r32 w = rf_table[n][d];

				current_pixel_r = image_result[j * image_width * image_channels + i * image_channels];
				current_pixel_g = image_result[j * image_width * image_channels + i * image_channels + 1];
				current_pixel_b = image_result[j * image_width * image_channels + i * image_channels + 2];

				last_pixel_r = ((1 - w) * current_pixel_r + w * last_pixel_r);
				last_pixel_g = ((1 - w) * current_pixel_g + w * last_pixel_g);
				last_pixel_b = ((1 - w) * current_pixel_b + w * last_pixel_b);

				image_result[j * image_width * image_channels + i * image_channels] = last_pixel_r;
				image_result[j * image_width * image_channels + i * image_channels + 1] = last_pixel_g;
				image_result[j * image_width * image_channels + i * image_channels + 2] = last_pixel_b;
				image_result[j * image_width * image_channels + i * image_channels + 3] = 1.0f;
			}
		}

		// Vertical Filter (Bottom-Up)
		for (s32 i = 0; i < image_width; ++i)
		{
			r32 last_pixel_r, last_pixel_g, last_pixel_b;
			r32 current_pixel_r, current_pixel_g, current_pixel_b;

			last_pixel_r = image_result[(image_height - 1) * image_width * image_channels + i * image_channels];
			last_pixel_g = image_result[(image_height - 1) * image_width * image_channels + i * image_channels + 1];
			last_pixel_b = image_result[(image_height - 1) * image_width * image_channels + i * image_channels + 2];

			for (s32 j = image_height - 1; j >= 0; --j)
			{
				s32 d_y_position = (j < image_height - 1) ? j + 1 : j;
				s32 d = vertical_domain_transforms[d_y_position * image_width + i];
				r32 w = rf_table[n][d];

				current_pixel_r = image_result[j * image_width * image_channels + i * image_channels];
				current_pixel_g = image_result[j * image_width * image_channels + i * image_channels + 1];
				current_pixel_b = image_result[j * image_width * image_channels + i * image_channels + 2];

				last_pixel_r = ((1 - w) * current_pixel_r + w * last_pixel_r);
				last_pixel_g = ((1 - w) * current_pixel_g + w * last_pixel_g);
				last_pixel_b = ((1 - w) * current_pixel_b + w * last_pixel_b);

				image_result[j * image_width * image_channels + i * image_channels] = last_pixel_r;
				image_result[j * image_width * image_channels + i * image_channels + 1] = last_pixel_g;
				image_result[j * image_width * image_channels + i * image_channels + 2] = last_pixel_b;
				image_result[j * image_width * image_channels + i * image_channels + 3] = 1.0f;
			}
		}
	}

	dealloc_memory(rf_coefficients);
	for (s32 i = 0; i < num_iterations; ++i)
		dealloc_memory(rf_table[i]);
	dealloc_memory(rf_table);
	dealloc_memory(vertical_domain_transforms);
	dealloc_memory(horizontal_domain_transforms);

	return 0;
}
