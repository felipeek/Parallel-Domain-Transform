#include "UI.h"
#include "Common.h"
#include "Util.h"
#include "Mathematics.h"
#include "Graphics.h"
#include "Input.h"
#include "OS.h"
#include <glad.h>
#include <stb_image.h>
#include "ParallelDomainTransform.h"

#define DEFAULT_CURSOR_SIZE 0.05f

#define DEFAULT_CURSOR_PATH "./res/cursor/arrow-pointer.png"
#define SELECT_BRUSH_CURSOR_PATH "./res/cursor/map-pin.png"
#define PAINT_CURSOR_PATH "./res/cursor/pencil-cursor.png"

#define RESET_IMAGE_PATH "./res/misc/reset.png"

#define BRUSH_C1_PATH "./res/brush/brush_c1.png"
#define BRUSH_C2_PATH "./res/brush/brush_c2.png"
#define BRUSH_C3_PATH "./res/brush/brush_c3.png"
#define BRUSH_S1_PATH "./res/brush/brush_s1.png"
#define BRUSH_S2_PATH "./res/brush/brush_s2.png"
#define BRUSH_S3_PATH "./res/brush/brush_s3.png"

#define BRUSHES_X 0.45f
#define BRUSHES_Y 0.2f
#define BRUSHES_WIDTH 0.12f
#define BRUSHES_HEIGHT 0.5f

#define Q_RESET_BUTTON_X 0.28f
#define Q_RESET_BUTTON_WIDTH 0.08f
#define Q_RESET_BUTTON_Y 0.3f
#define Q_RESET_BUTTON_HEIGHT 0.5f
#define Q_OF_BUTTON_X 0.05f
#define Q_OF_BUTTON_WIDTH 0.16f
#define Q_OF_BUTTON_Y 0.2f
#define Q_OF_BUTTON_HEIGHT 0.25f
#define Q_SV_BUTTON_X 0.05f
#define Q_SV_BUTTON_WIDTH 0.16f
#define Q_SV_BUTTON_Y 0.65f
#define Q_SV_BUTTON_HEIGHT 0.25f

#define Q_COLORS_X 0.625f
#define Q_COLORS_WIDTH 0.25f
#define Q_COLORS_Y 0.25f
#define Q_COLORS_HEIGHT 0.5f
#define Q_COLORS_COLUMNS 10
#define Q_COLORS_LINES 2

#define Q_SELECTED_COLOR_X 0.9f
#define Q_SELECTED_COLOR_Y 0.25f
#define Q_SELECTED_COLOR_WIDTH 0.07f
#define Q_SELECTED_COLOR_HEIGHT 0.5f

typedef enum Brush_Type_Enum
{
	BRUSH_TYPE_C1,
	BRUSH_TYPE_C2,
	BRUSH_TYPE_C3,
	BRUSH_TYPE_S1,
	BRUSH_TYPE_S2,
	BRUSH_TYPE_S3
} Brush_Type;

static Virtual_Window* vw_result;
static Virtual_Window* vw_scribbles;
static Virtual_Window* vw_menu;

// This flag is shared between the main thread and the DT thread.
// Whenever this is true, DT thread has finished its job and the filtered image is ready to be displayed
// Whenever this is false, DT thread will process or is processing the filtered image.
static bool dt_ready;
static bool dt_should_run;
// This flag is true when new scribbiles were added by the user.
// This is used to set dt_ready appropriately
static bool scribbles_added;

static bool is_image_loaded;
static r32* scribbles;
static r32* scribbles_for_dt_thread;	// this memory area will be used to send scribbles to the DT thread.
static r32* original_image;
static r32* result_image;
static r32* mask;
static s32 images_width;
static s32 images_height;

static Quad q_open_file;
static Quad q_save_file;
static Quad q_reset;

static Quad q_selected_brush;
static Quad q_brush_c1;
static Quad q_brush_c2;
static Quad q_brush_c3;
static Quad q_brush_s1;
static Quad q_brush_s2;
static Quad q_brush_s3;

static Quad q_result;
static Quad q_original_image;
static Quad q_scribbles;
static Quad q_colors[Q_COLORS_LINES][Q_COLORS_COLUMNS];
static Quad q_selected_color;

static Quad q_default_cursor;
static Quad q_select_brush_cursor;
static Quad q_paint_cursor;

static Quad* q_selected_cursor;

static Text t_load_image;
static Text t_save_image;
static Text t_reset;
static Text t_brushes;
static Text t_colors;
static Text t_selected_color;

static Brush_Type selected_brush = BRUSH_TYPE_S1;

static Mutex_Handler mutex;

static bool force_reset_scribbles = false;

static r32* generate_scribbles_mask(const r32* scribbles_bytes,
	s32 scribbles_width,
	s32 scribbles_height,
	s32 scribbles_channels,
	r32* mask)
{
	for (s32 i = 0; i < scribbles_height; ++i)
		for (s32 j = 0; j < scribbles_width; ++j)
		{
			r32 r = scribbles_bytes[i * scribbles_width * scribbles_channels + j * scribbles_channels];
			r32 g = scribbles_bytes[i * scribbles_width * scribbles_channels + j * scribbles_channels + 1];
			r32 b = scribbles_bytes[i * scribbles_width * scribbles_channels + j * scribbles_channels + 2];

			if (r != 0.0f || g != 0.0f || b != 0.0f)
			{
				mask[i * scribbles_width * scribbles_channels + j * scribbles_channels] = 1.0f;
				mask[i * scribbles_width * scribbles_channels + j * scribbles_channels + 1] = 1.0f;
				mask[i * scribbles_width * scribbles_channels + j * scribbles_channels + 2] = 1.0f;
				mask[i * scribbles_width * scribbles_channels + j * scribbles_channels + 3] = 1.0f;
			}
			else
			{
				mask[i * scribbles_width * scribbles_channels + j * scribbles_channels] = 0.0f;
				mask[i * scribbles_width * scribbles_channels + j * scribbles_channels + 1] = 0.0f;
				mask[i * scribbles_width * scribbles_channels + j * scribbles_channels + 2] = 0.0f;
				mask[i * scribbles_width * scribbles_channels + j * scribbles_channels + 3] = 1.0f;
			}
		}

	return mask;
}

Thread_Proc_Return_Type _stdcall dt_thread(void* prm)
{
	while (1)
	{
		if (!dt_ready)
		{
			r32* mask_bytes = generate_scribbles_mask(scribbles_for_dt_thread, images_width, images_height, 4, mask);
			start_clock();
			parallel_colorization(original_image, scribbles_for_dt_thread, mask_bytes, images_width, images_height, 4, 200, 15, 4, 4, 2, 2, result_image);
			r32 total_time = end_clock();
			print("Total time: %.3f seconds\n", total_time);
			lock_mutex(mutex);
			dt_ready = true;
			release_mutex(mutex);
		}
		Sleep(10);
	}
}

static void create_dt_thread()
{
	mutex = create_mutex();
	dt_ready = false;
	scribbles_added = false;
	create_thread(dt_thread, 0);
}

static void load_shaders()
{
}

static void fill_image_structures(const s8* path)
{
	Vec4 position = { 0.0f, 0.0f, 0.0f, 1.0f };
	Vec3 scale = { 1.0f, 1.0f, 0.0f };

	if (is_image_loaded)
	{
		dealloc_memory(original_image);
		dealloc_memory(result_image);
		dealloc_memory(scribbles);
		dealloc_memory(mask);
		dealloc_memory(scribbles_for_dt_thread);
	}

	s32 image_channels;

	stbi_set_flip_vertically_on_load(1);
	original_image = load_image(path, &images_width, &images_height, &image_channels, 4);
	graphics_quad_create(position, scale, original_image, images_width, images_height, &q_result);

	result_image = load_image(path, &images_width, &images_height, &image_channels, 4);
	graphics_quad_create(position, scale, result_image, images_width, images_height, &q_original_image);

	scribbles_for_dt_thread = (r32*)alloc_memory(images_width * images_height * image_channels * sizeof(r32));
	memory_set(scribbles_for_dt_thread, 0, images_width * images_height * image_channels * sizeof(r32));

	scribbles = (r32*)alloc_memory(images_width * images_height * image_channels * sizeof(r32));
	memory_set(scribbles, 0, images_width * images_height * image_channels * sizeof(r32));

	mask = (r32*)alloc_memory(images_width * images_height * image_channels * sizeof(r32));

	graphics_quad_create(position, scale, scribbles, images_width, images_height, &q_scribbles);

	lock_mutex(mutex);
	dt_ready = false;
	release_mutex(mutex);
	scribbles_added = false;
}

static void create_texts()
{
	t_load_image = graphics_text_create(3, 0, 0, 0, 0);
	t_save_image = graphics_text_create(4, 0, 0, 0, 0);
	t_reset = graphics_text_create(5, 0, 0, 0, 0);
	t_brushes = graphics_text_create(6, 0, 0, 0, 0);
	t_colors = graphics_text_create(7, 0, 0, 0, 0);
	t_selected_color = graphics_text_create(8, 0, 0, 0, 0);
}

static void create_brush_quads()
{
	stbi_set_flip_vertically_on_load(1);
	int image_width, image_height, image_channels;

	r32* image_data = load_image(BRUSH_C1_PATH, &image_width, &image_height, &image_channels, 4);
	graphics_quad_create({ 0.0f, 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, image_data,
		image_width, image_height, &q_brush_c1);
	dealloc_memory(image_data);

	image_data = load_image(BRUSH_C2_PATH, &image_width, &image_height, &image_channels, 4);
	graphics_quad_create({ 0.0f, 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, image_data,
		image_width, image_height, &q_brush_c2);
	dealloc_memory(image_data);

	image_data = load_image(BRUSH_C3_PATH, &image_width, &image_height, &image_channels, 4);
	graphics_quad_create({ 0.0f, 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, image_data,
		image_width, image_height, &q_brush_c3);
	dealloc_memory(image_data);

	image_data = load_image(BRUSH_S1_PATH, &image_width, &image_height, &image_channels, 4);
	graphics_quad_create({ 0.0f, 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, image_data,
		image_width, image_height, &q_brush_s1);
	dealloc_memory(image_data);

	image_data = load_image(BRUSH_S2_PATH, &image_width, &image_height, &image_channels, 4);
	graphics_quad_create({ 0.0f, 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, image_data,
		image_width, image_height, &q_brush_s2);
	dealloc_memory(image_data);

	image_data = load_image(BRUSH_S3_PATH, &image_width, &image_height, &image_channels, 4);
	graphics_quad_create({ 0.0f, 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, image_data,
		image_width, image_height, &q_brush_s3);
	dealloc_memory(image_data);

	graphics_quad_create({ 0.0f, 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 0.3f }, &q_selected_brush);
}

static void create_cursor_quads()
{
	stbi_set_flip_vertically_on_load(1);
	int image_width, image_height, image_channels;

	r32* image_data = load_image(DEFAULT_CURSOR_PATH, &image_width, &image_height, &image_channels, 4);
	graphics_quad_create({ 0.0f, 0.0f, 0.0f, 1.0f }, { DEFAULT_CURSOR_SIZE, DEFAULT_CURSOR_SIZE, DEFAULT_CURSOR_SIZE }, image_data,
		image_width, image_height, &q_default_cursor);
	dealloc_memory(image_data);

	image_data = load_image(SELECT_BRUSH_CURSOR_PATH, &image_width, &image_height, &image_channels, 4);
	graphics_quad_create({ 0.0f, 0.0f, 0.0f, 1.0f }, { DEFAULT_CURSOR_SIZE, DEFAULT_CURSOR_SIZE, DEFAULT_CURSOR_SIZE }, image_data,
		image_width, image_height, &q_select_brush_cursor);
	dealloc_memory(image_data);

	image_data = load_image(PAINT_CURSOR_PATH, &image_width, &image_height, &image_channels, 4);
	graphics_quad_create({ 0.0f, 0.0f, 0.0f, 1.0f }, { DEFAULT_CURSOR_SIZE, DEFAULT_CURSOR_SIZE, DEFAULT_CURSOR_SIZE }, image_data,
		image_width, image_height, &q_paint_cursor);
	dealloc_memory(image_data);

	q_selected_cursor = &q_default_cursor;
}

static void create_color_quads()
{
	/* FILL Q_COLORS_COLUMNS x Q_COLORS_LINES quads */
	graphics_quad_create({ 0.0f, 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f / 255.0f, 0.0f / 255.0f, 0.0f / 255.0f, 1.0f }, &q_colors[0][0]);
	graphics_quad_create({ 0.0f, 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, { 127.0f / 255.0f, 127.0f / 255.0f, 0.0f / 127.0f, 1.0f }, &q_colors[0][1]);
	graphics_quad_create({ 0.0f, 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, { 136.0f / 255.0f, 0.0f / 255.0f, 21.0f / 255.0f, 1.0f }, &q_colors[0][2]);
	graphics_quad_create({ 0.0f, 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, { 237.0f / 255.0f, 28.0f / 255.0f, 36.0f / 255.0f, 1.0f }, &q_colors[0][3]);
	graphics_quad_create({ 0.0f, 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, { 255.0f / 255.0f, 127.0f / 255.0f, 39.0f / 255.0f, 1.0f }, &q_colors[0][4]);
	graphics_quad_create({ 0.0f, 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, { 255.0f / 255.0f, 242.0f / 255.0f, 0.0f / 255.0f, 1.0f }, &q_colors[0][5]);
	graphics_quad_create({ 0.0f, 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, { 34.0f / 255.0f, 177.0f / 255.0f, 76.0f / 255.0f, 1.0f }, &q_colors[0][6]);
	graphics_quad_create({ 0.0f, 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f / 255.0f, 162.0f / 255.0f, 232.0f / 255.0f, 1.0f }, &q_colors[0][7]);
	graphics_quad_create({ 0.0f, 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, { 63.0f / 255.0f, 72.0f / 255.0f, 204.0f / 255.0f, 1.0f }, &q_colors[0][8]);
	graphics_quad_create({ 0.0f, 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, { 163.0f / 255.0f, 73.0f / 255.0f, 164.0f / 255.0f, 1.0f }, &q_colors[0][9]);
	graphics_quad_create({ 0.0f, 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, { 255.0f / 255.0f, 255.0f / 255.0f, 255.0f / 255.0f, 1.0f }, &q_colors[1][0]);
	graphics_quad_create({ 0.0f, 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, { 195.0f / 255.0f, 195.0f / 255.0f, 195.0f / 255.0f, 1.0f }, &q_colors[1][1]);
	graphics_quad_create({ 0.0f, 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, { 185.0f / 255.0f, 122.0f / 255.0f, 87.0f / 255.0f, 1.0f }, &q_colors[1][2]);
	graphics_quad_create({ 0.0f, 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, { 255.0f / 255.0f, 174.0f / 255.0f, 201.0f / 255.0f, 1.0f }, &q_colors[1][3]);
	graphics_quad_create({ 0.0f, 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, { 255.0f / 255.0f, 201.0f / 255.0f, 14.0f / 255.0f, 1.0f }, &q_colors[1][4]);
	graphics_quad_create({ 0.0f, 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, { 239.0f / 255.0f, 228.0f / 255.0f, 176.0f / 255.0f, 1.0f }, &q_colors[1][5]);
	graphics_quad_create({ 0.0f, 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, { 181.0f / 255.0f, 230.0f / 255.0f, 29.0f / 255.0f, 1.0f }, &q_colors[1][6]);
	graphics_quad_create({ 0.0f, 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, { 153.0f / 255.0f, 217.0f / 255.0f, 234.0f / 255.0f, 1.0f }, &q_colors[1][7]);
	graphics_quad_create({ 0.0f, 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, { 112.0f / 255.0f, 146.0f / 255.0f, 190.0f / 255.0f, 1.0f }, &q_colors[1][8]);
	graphics_quad_create({ 0.0f, 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, { 200.0f / 255.0f, 191.0f / 255.0f, 231.0f / 255.0f, 1.0f }, &q_colors[1][9]);

	graphics_quad_create({ 0.0f, 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f, 1.0f }, &q_selected_color);
}

static void create_button_quads()
{
	graphics_quad_create({ 0.0f, 0.0f, 0.0f, 1.0f }, { 1.0f, 0.5f, 1.0f }, { 200.0f / 255.0f, 0.0f / 255.0f, 200.0f / 255.0f, 0.3f }, &q_open_file);
	graphics_quad_create({ 0.0f, 0.0f, 0.0f, 1.0f }, { 1.0f, 0.5f, 1.0f }, { 200.0f / 255.0f, 0.0f / 255.0f, 200.0f / 255.0f, 0.3f }, &q_save_file);
	
	stbi_set_flip_vertically_on_load(1);
	int image_width, image_height, image_channels;
	r32* image_data = load_image(RESET_IMAGE_PATH, &image_width, &image_height, &image_channels, 4);
	graphics_quad_create({ 0.0f, 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, image_data,
		image_width, image_height, &q_reset);
	dealloc_memory(image_data);
}

static void update_menu(Virtual_Window* vw)
{
	Vec4 position;
	Vec3 scale;

	r32 q_colors_screen_pos_x = vw->drawable_quad.position.x + Q_COLORS_X * vw->drawable_quad.scale.x;
	r32 q_colors_screen_pos_y = vw->drawable_quad.position.y - Q_COLORS_Y * vw->drawable_quad.scale.y;
	r32 q_colors_screen_width = vw->drawable_quad.scale.x * Q_COLORS_WIDTH;
	r32 q_colors_screen_height = vw->drawable_quad.scale.y * Q_COLORS_HEIGHT;
	
	if (graphics_can_virtual_window_process_input(vw))
	{
		// Check if color was selected
		if (input_data.mclicked)
		{
			for (s32 i = 0; i < Q_COLORS_LINES; ++i)
				for (s32 j = 0; j < Q_COLORS_COLUMNS; ++j)
				{
					Quad* q = &q_colors[i][j];
					if (input_data.mx >= q->position.x && input_data.mx <= (q->position.x + q->scale.x))
						if (input_data.my <= q->position.y && input_data.my >= (q->position.y - q->scale.y))
							q_selected_color.color = q->color;
				}
		}
	}

	// Update Colors position and Scale
	for (s32 i = 0; i < Q_COLORS_LINES; ++i)
		for (s32 j = 0; j < Q_COLORS_COLUMNS; ++j)
		{
			position.x = q_colors_screen_pos_x + j * (q_colors_screen_width / Q_COLORS_COLUMNS);
			position.y = q_colors_screen_pos_y - i * (q_colors_screen_height / Q_COLORS_LINES);
			position.z = 0.0f;
			position.w = 1.0f;

			scale.x = q_colors_screen_width / Q_COLORS_COLUMNS;
			scale.y = q_colors_screen_height / Q_COLORS_LINES;
			scale.z = 1.0f;

			q_colors[i][j].position = position;
			q_colors[i][j].scale = scale;
		}

	position.x = vw->drawable_quad.position.x + Q_SELECTED_COLOR_X * vw->drawable_quad.scale.x;
	position.y = vw->drawable_quad.position.y - Q_SELECTED_COLOR_Y * vw->drawable_quad.scale.y;
	position.z = 0.0f;
	position.w = 1.0f;
	scale = { vw->drawable_quad.scale.x * Q_SELECTED_COLOR_WIDTH, vw->drawable_quad.scale.y * Q_SELECTED_COLOR_HEIGHT, 1.0f };
	q_selected_color.position = position;
	q_selected_color.scale = scale;

	// Update buttons
	const s32 buffer_size = 1024;
	s8 buffer[buffer_size];

	// Check if button is clicked
	if (graphics_can_virtual_window_process_input(vw))
	{
		// Check if color was selected
		if (input_data.mclicked)
		{
			if (input_data.mx >= q_open_file.position.x && input_data.mx <= (q_open_file.position.x + q_open_file.scale.x))
				if (input_data.my <= q_open_file.position.y && input_data.my >= (q_open_file.position.y - q_open_file.scale.y))
					if (os_open_file_dialog(buffer, buffer_size))
						fill_image_structures(buffer);

			if (input_data.mx >= q_save_file.position.x && input_data.mx <= (q_save_file.position.x + q_save_file.scale.x))
				if (input_data.my <= q_save_file.position.y && input_data.my >= (q_save_file.position.y - q_save_file.scale.y))
					if (os_save_file_dialog(buffer, buffer_size))
					{
						store_image(buffer, images_width, images_height, 4, result_image);
					}

			if (input_data.mx >= q_reset.position.x && input_data.mx <= (q_reset.position.x + q_reset.scale.x))
				if (input_data.my <= q_reset.position.y && input_data.my >= (q_reset.position.y - q_reset.scale.y))
					force_reset_scribbles = true;
		}
	}

	// Update Open File Button position and scale
	position.x = vw->drawable_quad.position.x + Q_OF_BUTTON_X * vw->drawable_quad.scale.x;
	position.y = vw->drawable_quad.position.y - Q_OF_BUTTON_Y * vw->drawable_quad.scale.y;
	position.z = 0.0f;
	position.w = 1.0f;
	scale = { vw->drawable_quad.scale.x * Q_OF_BUTTON_WIDTH, vw->drawable_quad.scale.y * Q_OF_BUTTON_HEIGHT, 1.0f };
	q_open_file.position = position;
	q_open_file.scale = scale;

	// Update Save File Button position and scale
	position.x = vw->drawable_quad.position.x + Q_SV_BUTTON_X * vw->drawable_quad.scale.x;
	position.y = vw->drawable_quad.position.y - Q_SV_BUTTON_Y * vw->drawable_quad.scale.y;
	position.z = 0.0f;
	position.w = 1.0f;
	scale = { vw->drawable_quad.scale.x * Q_SV_BUTTON_WIDTH, vw->drawable_quad.scale.y * Q_SV_BUTTON_HEIGHT, 1.0f };
	q_save_file.position = position;
	q_save_file.scale = scale;

	// Update Reset Button position and scale
	position.x = vw->drawable_quad.position.x + Q_RESET_BUTTON_X * vw->drawable_quad.scale.x;
	position.y = vw->drawable_quad.position.y - Q_RESET_BUTTON_Y * vw->drawable_quad.scale.y;
	position.z = 0.0f;
	position.w = 1.0f;
	scale = { vw->drawable_quad.scale.x * Q_RESET_BUTTON_WIDTH, vw->drawable_quad.scale.y * Q_RESET_BUTTON_HEIGHT, 1.0f };
	q_reset.position = position;
	q_reset.scale = scale;

	// Update brushes

	if (graphics_can_virtual_window_process_input(vw))
	{
		// Check if brush was selected
		if (input_data.mclicked)
		{
			if (input_data.mx >= q_brush_c1.position.x && input_data.mx <= (q_brush_c1.position.x + q_brush_c1.scale.x))
				if (input_data.my <= q_brush_c1.position.y && input_data.my >= (q_brush_c1.position.y - q_brush_c1.scale.y))
					selected_brush = BRUSH_TYPE_C1;
			if (input_data.mx >= q_brush_c2.position.x && input_data.mx <= (q_brush_c2.position.x + q_brush_c2.scale.x))
				if (input_data.my <= q_brush_c2.position.y && input_data.my >= (q_brush_c2.position.y - q_brush_c2.scale.y))
					selected_brush = BRUSH_TYPE_C2;
			if (input_data.mx >= q_brush_c3.position.x && input_data.mx <= (q_brush_c3.position.x + q_brush_c3.scale.x))
				if (input_data.my <= q_brush_c3.position.y && input_data.my >= (q_brush_c3.position.y - q_brush_c3.scale.y))
					selected_brush = BRUSH_TYPE_C3;
			if (input_data.mx >= q_brush_s1.position.x && input_data.mx <= (q_brush_s1.position.x + q_brush_s1.scale.x))
				if (input_data.my <= q_brush_s1.position.y && input_data.my >= (q_brush_s1.position.y - q_brush_s1.scale.y))
					selected_brush = BRUSH_TYPE_S1;
			if (input_data.mx >= q_brush_s2.position.x && input_data.mx <= (q_brush_s2.position.x + q_brush_s2.scale.x))
				if (input_data.my <= q_brush_s2.position.y && input_data.my >= (q_brush_s2.position.y - q_brush_s2.scale.y))
					selected_brush = BRUSH_TYPE_S2;
			if (input_data.mx >= q_brush_s3.position.x && input_data.mx <= (q_brush_s3.position.x + q_brush_s3.scale.x))
				if (input_data.my <= q_brush_s3.position.y && input_data.my >= (q_brush_s3.position.y - q_brush_s3.scale.y))
					selected_brush = BRUSH_TYPE_S3;
		}
	}

	r32 brushes_screen_pos_x = vw->drawable_quad.position.x + BRUSHES_X * vw->drawable_quad.scale.x;
	r32 brushes_screen_pos_y = vw->drawable_quad.position.y - BRUSHES_Y * vw->drawable_quad.scale.y;
	r32 brushes_screen_width = vw->drawable_quad.scale.x * BRUSHES_WIDTH;
	r32 brushes_screen_height = vw->drawable_quad.scale.y * BRUSHES_HEIGHT;

	scale = { brushes_screen_width / 3, brushes_screen_height / 2, 1.0f };

	position.x = brushes_screen_pos_x + 0 * (brushes_screen_width / 3);
	position.y = q_colors_screen_pos_y - 0 * (q_colors_screen_height / 2);
	position.z = 0.0f;
	position.w = 1.0f;
	q_brush_c1.position = position;
	q_brush_c1.scale = scale;

	if (selected_brush == BRUSH_TYPE_C1)
	{
		q_selected_brush.position = position;
		q_selected_brush.scale = scale;
	}

	position.x = brushes_screen_pos_x + 1 * (brushes_screen_width / 3);
	position.y = q_colors_screen_pos_y - 0 * (q_colors_screen_height / 2);
	position.z = 0.0f;
	position.w = 1.0f;
	q_brush_c2.position = position;
	q_brush_c2.scale = scale;

	if (selected_brush == BRUSH_TYPE_C2)
	{
		q_selected_brush.position = position;
		q_selected_brush.scale = scale;
	}

	position.x = brushes_screen_pos_x + 2 * (brushes_screen_width / 3);
	position.y = q_colors_screen_pos_y - 0 * (q_colors_screen_height / 2);
	position.z = 0.0f;
	position.w = 1.0f;
	q_brush_c3.position = position;
	q_brush_c3.scale = scale;

	if (selected_brush == BRUSH_TYPE_C3)
	{
		q_selected_brush.position = position;
		q_selected_brush.scale = scale;
	}

	position.x = brushes_screen_pos_x + 0 * (brushes_screen_width / 3);
	position.y = q_colors_screen_pos_y - 1 * (q_colors_screen_height / 2);
	position.z = 0.0f;
	position.w = 1.0f;
	q_brush_s1.position = position;
	q_brush_s1.scale = scale;

	if (selected_brush == BRUSH_TYPE_S1)
	{
		q_selected_brush.position = position;
		q_selected_brush.scale = scale;
	}

	position.x = brushes_screen_pos_x + 1 * (brushes_screen_width / 3);
	position.y = q_colors_screen_pos_y - 1 * (q_colors_screen_height / 2);
	position.z = 0.0f;
	position.w = 1.0f;
	q_brush_s2.position = position;
	q_brush_s2.scale = scale;

	if (selected_brush == BRUSH_TYPE_S2)
	{
		q_selected_brush.position = position;
		q_selected_brush.scale = scale;
	}

	position.x = brushes_screen_pos_x + 2 * (brushes_screen_width / 3);
	position.y = q_colors_screen_pos_y - 1 * (q_colors_screen_height / 2);
	position.z = 0.0f;
	position.w = 1.0f;
	q_brush_s3.position = position;
	q_brush_s3.scale = scale;

	if (selected_brush == BRUSH_TYPE_S3)
	{
		q_selected_brush.position = position;
		q_selected_brush.scale = scale;
	}

	// Update All Menu Texts
	t_load_image.x = vw->drawable_quad.position.x + vw->drawable_quad.scale.x * Q_OF_BUTTON_X;
	t_load_image.y = vw->drawable_quad.position.y - vw->drawable_quad.scale.y * Q_OF_BUTTON_Y;
	t_load_image.width = vw->drawable_quad.scale.x * Q_OF_BUTTON_WIDTH;
	t_load_image.height = vw->drawable_quad.scale.y * Q_OF_BUTTON_HEIGHT;
	t_save_image.x = vw->drawable_quad.position.x + vw->drawable_quad.scale.x * Q_SV_BUTTON_X;
	t_save_image.y = vw->drawable_quad.position.y - vw->drawable_quad.scale.y * Q_SV_BUTTON_Y;
	t_save_image.width = vw->drawable_quad.scale.x * Q_SV_BUTTON_WIDTH;
	t_save_image.height = vw->drawable_quad.scale.y * Q_SV_BUTTON_HEIGHT;
	t_reset.x = vw->drawable_quad.position.x + vw->drawable_quad.scale.x * Q_RESET_BUTTON_X;
	t_reset.y = vw->drawable_quad.position.y - vw->drawable_quad.scale.y * (Q_RESET_BUTTON_Y - 0.25f);
	t_reset.width = vw->drawable_quad.scale.x * Q_RESET_BUTTON_WIDTH;
	t_reset.height = vw->drawable_quad.scale.y * Q_RESET_BUTTON_HEIGHT;
	t_brushes.x = vw->drawable_quad.position.x + vw->drawable_quad.scale.x * BRUSHES_X;
	t_brushes.y = vw->drawable_quad.position.y - vw->drawable_quad.scale.y * (BRUSHES_Y - 0.15f);
	t_brushes.width = vw->drawable_quad.scale.x * BRUSHES_WIDTH;
	t_brushes.height = vw->drawable_quad.scale.y * BRUSHES_HEIGHT;
	t_colors.x = vw->drawable_quad.position.x + vw->drawable_quad.scale.x * Q_COLORS_X;
	t_colors.y = vw->drawable_quad.position.y - vw->drawable_quad.scale.y * (Q_COLORS_Y - 0.20f);
	t_colors.width = vw->drawable_quad.scale.x * Q_COLORS_WIDTH;
	t_colors.height = vw->drawable_quad.scale.y * Q_COLORS_HEIGHT;
	t_selected_color.x = vw->drawable_quad.position.x + vw->drawable_quad.scale.x * Q_SELECTED_COLOR_X;
	t_selected_color.y = vw->drawable_quad.position.y - vw->drawable_quad.scale.y * Q_SELECTED_COLOR_Y;
	t_selected_color.width = vw->drawable_quad.scale.x * Q_SELECTED_COLOR_WIDTH;
	t_selected_color.height = vw->drawable_quad.scale.y * Q_SELECTED_COLOR_HEIGHT;
}

static void render_menu(Virtual_Window* vw)
{
	graphics_quad_render(&q_brush_c1, quad_shader, GL_TRIANGLES);
	graphics_quad_render(&q_brush_c2, quad_shader, GL_TRIANGLES);
	graphics_quad_render(&q_brush_c3, quad_shader, GL_TRIANGLES);
	graphics_quad_render(&q_brush_s1, quad_shader, GL_TRIANGLES);
	graphics_quad_render(&q_brush_s2, quad_shader, GL_TRIANGLES);
	graphics_quad_render(&q_brush_s3, quad_shader, GL_TRIANGLES);
	graphics_quad_render(&q_selected_brush, quad_shader, GL_TRIANGLES);

	graphics_quad_render(&q_reset, quad_shader, GL_TRIANGLES);
	graphics_quad_render(&q_save_file, quad_shader, GL_TRIANGLES);
	graphics_quad_render(&q_open_file, quad_shader, GL_TRIANGLES);

	for (s32 i = 0; i < Q_COLORS_LINES; ++i)
		for (s32 j = 0; j < Q_COLORS_COLUMNS; ++j)
			graphics_quad_render(&q_colors[i][j], quad_shader, GL_TRIANGLES);

	graphics_quad_render(&q_selected_color, quad_shader, GL_TRIANGLES);

	graphics_text_render(&t_load_image, quad_shader);
	graphics_text_render(&t_save_image, quad_shader);
	graphics_text_render(&t_reset, quad_shader);
	graphics_text_render(&t_brushes, quad_shader);
	graphics_text_render(&t_colors, quad_shader);
	//graphics_text_render(&t_selected_color, quad_shader);
}

static void fill_scribble_pixel(s32 x, s32 y)
{
	if (selected_brush == BRUSH_TYPE_S1 || selected_brush == BRUSH_TYPE_S2 || selected_brush == BRUSH_TYPE_S3)
	{
		s32 brush_size;

		switch (selected_brush)
		{
			case BRUSH_TYPE_S1: {
				brush_size = 9;
			} break;
			case BRUSH_TYPE_S2: {
				brush_size = 5;
			} break;
			default:
			case BRUSH_TYPE_S3: {
				brush_size = 3;
			} break;
		}

		for (s32 i = y - brush_size; i < y + brush_size; ++i)
			for (s32 j = x - brush_size; j < x + brush_size; ++j)
			{
				if (i < 0 || i >= images_height || j < 0 || j >= images_width)
					continue;
				scribbles[i * images_width * 4 + j * 4] = q_selected_color.color.x;
				scribbles[i * images_width * 4 + j * 4 + 1] = q_selected_color.color.y;
				scribbles[i * images_width * 4 + j * 4 + 2] = q_selected_color.color.z;
				scribbles[i * images_width * 4 + j * 4 + 3] = 1.0f;
			}
	}

	if (selected_brush == BRUSH_TYPE_C1 || selected_brush == BRUSH_TYPE_C2 || selected_brush == BRUSH_TYPE_C3)
	{
		s32 brush_size;

		switch (selected_brush)
		{
		case BRUSH_TYPE_C1: {
			brush_size = 9;
		} break;
		case BRUSH_TYPE_C2: {
			brush_size = 5;
		} break;
		default:
		case BRUSH_TYPE_C3: {
			brush_size = 3;
		} break;
		}

		for (s32 i = y - brush_size; i < y + brush_size; ++i)
			for (s32 j = x - brush_size; j < x + brush_size; ++j)
			{
				if (i < 0 || i >= images_height || j < 0 || j >= images_width || r32_sqrt((y-i) * (y-i) + (x-j) * (x-j)) > brush_size)
					continue;
				scribbles[i * images_width * 4 + j * 4] = q_selected_color.color.x;
				scribbles[i * images_width * 4 + j * 4 + 1] = q_selected_color.color.y;
				scribbles[i * images_width * 4 + j * 4 + 2] = q_selected_color.color.z;
				scribbles[i * images_width * 4 + j * 4 + 3] = 1.0f;
			}
	}
}

// Uses bressenham algorithm
static void fill_scribbles_pixels(s32 x1, s32 y1, s32 x2, s32 y2)
{
	if (x1 == x2 && y1 == y2)
		return;

	if (x1 > x2)
	{
		fill_scribbles_pixels(x2, y2, x1, y1);
		return;
	}

	r32 m;
	
	if (x2 == x1)
	{
		if (y2 > y1)
			m = 10.0f;
		else
			m = -10.0f;
	}
	else
		m = ((r32)y2 - y1) / ((r32)x2 - x1);

	if (0.0f <= m && m < 1.0f)
	{
		s32 dy = y2 - y1;
		s32 dx = x2 - x1;
		s32 x = x1;
		s32 y = y1;
		s32 E = 2 * dy - dx;

		while (x <= x2)
		{
			fill_scribble_pixel(x, y);
			++x;
			if (E >= 0)
			{
				++y;
				E = E - 2 * dx;
			}
			E = E + 2 * dy;
		}
	}
	else if (-1.0f <= m && m < 0.0f)
	{
		s32 dy = y2 - y1;
		s32 dx = x2 - x1;
		s32 x = x1;
		s32 y = y1;
		s32 E = 2 * dy - dx;

		while (x <= x2)
		{
			fill_scribble_pixel(x, y);
			++x;
			if (E >= 0)
			{
				--y;
				E = E - 2 * dx;
			}
			E = E - 2 * dy;
		}
	}
	else if (m >= 1.0f)
	{
		s32 dy = y2 - y1;
		s32 dx = x2 - x1;
		s32 x = x1;
		s32 y = y1;
		s32 E = 2 * dy - dx;

		while (y <= y2)
		{
			fill_scribble_pixel(x, y);
			++y;
			if (E >= 0)
			{
				++x;
				E = E - 2 * dy;
			}
			E = E + 2 * dx;
		}
	}
	else if (m < 1.0f)
	{
		s32 dy = y2 - y1;
		s32 dx = x2 - x1;
		s32 x = x1;
		s32 y = y1;
		s32 E = 2 * dy - dx;

		while (y >= y2)
		{
			fill_scribble_pixel(x, y);
			--y;
			if (E >= 0)
			{
				++x;
				E = E + 2 * dy;
			}
			E = E + 2 * dx;
		}
	}
}

static void update_image_windows(Virtual_Window* vw)
{
	if (vw == vw_result)
	{
		q_result.position = { vw->drawable_quad.position.x, vw->drawable_quad.position.y, 0.0f, 1.0f };
		q_result.scale = { vw->drawable_quad.scale.x, vw->drawable_quad.scale.y, 0.0f };
	}
	else if (vw == vw_scribbles)
	{
		q_original_image.position = { vw->drawable_quad.position.x, vw->drawable_quad.position.y, 0.0f, 1.0f };
		q_original_image.scale = { vw->drawable_quad.scale.x, vw->drawable_quad.scale.y, 0.0f };
		q_scribbles.position = { vw->drawable_quad.position.x, vw->drawable_quad.position.y, 0.0f, 1.0f };
		q_scribbles.scale = { vw->drawable_quad.scale.x, vw->drawable_quad.scale.y, 0.0f };

		// Used to interpolate scribbles
		static bool last_frame_was_drawing = false;

		if (force_reset_scribbles)
		{
			memory_set(scribbles, 0, images_width * images_height * 4 * sizeof(r32));
			graphics_update_quad_texture(&q_scribbles, scribbles, images_width, images_height);

			copy_memory(scribbles_for_dt_thread, scribbles, images_width * images_height * 4 * sizeof(r32));
			scribbles_added = true;
			last_frame_was_drawing = false;
			force_reset_scribbles = false;
		}
		else if (input_data.mclicked && graphics_can_virtual_window_process_input(vw))
		{
			r32 image_x_click = (input_data.mx - vw->drawable_quad.position.x) / vw->drawable_quad.scale.x;
			r32 image_y_click = (input_data.my - vw->drawable_quad.position.y + vw->drawable_quad.scale.y) / vw->drawable_quad.scale.y;

			static s32 last_selected_x_pixel = 0;
			static s32 last_selected_y_pixel = 0;
			s32 selected_x_pixel = r32_round(image_x_click * images_width);
			s32 selected_y_pixel = r32_round(image_y_click * images_height);

			if (last_frame_was_drawing)
				fill_scribbles_pixels(last_selected_x_pixel, last_selected_y_pixel, selected_x_pixel, selected_y_pixel);
			else
				fill_scribble_pixel(selected_x_pixel, selected_y_pixel);

			graphics_update_quad_texture(&q_scribbles, scribbles, images_width, images_height);

			copy_memory(scribbles_for_dt_thread, scribbles, images_width * images_height * 4 * sizeof(r32));
			scribbles_added = true;
			last_selected_x_pixel = selected_x_pixel;
			last_selected_y_pixel = selected_y_pixel;
			last_frame_was_drawing = true;
			//print("image_x_click: %.2f\nimage_y_click: %.2f\n", image_x_click, image_y_click);
		}
		else
			last_frame_was_drawing = false;
	}
}

static void render_image_windows(Virtual_Window* vw)
{
	if (vw == vw_result)
	{
		graphics_quad_render(&q_result, quad_shader, GL_TRIANGLES);
	}
	else if (vw == vw_scribbles)
	{
		graphics_quad_render(&q_original_image, quad_shader, GL_TRIANGLES);
		graphics_quad_render(&q_scribbles, quad_shader, GL_TRIANGLES);
	}
}

extern void ui_init()
{
	create_dt_thread();
	graphics_init();
	create_brush_quads();
	create_color_quads();
	create_cursor_quads();
	create_button_quads();
	create_texts();
	fill_image_structures("./res/fallen.png");

	vw_result = graphics_virtual_window_create(-1.0f, 1.0f, 1.0f, 1.0f, render_image_windows, update_image_windows, true, 0);
	vw_scribbles = graphics_virtual_window_create(0.0f, 1.0f, 1.0f, 1.0f, render_image_windows, update_image_windows, true, 1);
	vw_menu = graphics_virtual_window_create(-1.0f, -0.6f, 2.0f, 0.4f, render_menu, update_menu, true, 2);

	load_shaders();
	glClearColor(0.7f, 0.7f, 0.7f, 1.0f);
}

extern void ui_render()
{
	glClear(GL_COLOR_BUFFER_BIT);
	graphics_virtual_windows_render();
	graphics_quad_render(q_selected_cursor, quad_shader, GL_TRIANGLES);
}

extern void ui_update()
{
	q_selected_cursor->position = { input_data.mx, input_data.my, 0.0f, 1.0f };
	graphics_virtual_windows_update();

	r32 q_colors_screen_pos_x = vw_menu->drawable_quad.position.x + Q_COLORS_X * vw_menu->drawable_quad.scale.x;
	r32 q_colors_screen_pos_y = vw_menu->drawable_quad.position.y - Q_COLORS_Y * vw_menu->drawable_quad.scale.y;
	r32 q_colors_screen_width = vw_menu->drawable_quad.scale.x * Q_COLORS_WIDTH;
	r32 q_colors_screen_height = vw_menu->drawable_quad.scale.y * Q_COLORS_HEIGHT;

	// Cursor Selection Logic
	q_selected_cursor = &q_default_cursor;

	if (graphics_can_virtual_window_process_input(vw_scribbles))
		q_selected_cursor = &q_paint_cursor;

	if (graphics_can_virtual_window_process_input(vw_menu))
		if (input_data.mx >= q_colors_screen_pos_x && input_data.mx <= (q_colors_screen_pos_x + q_colors_screen_width))
			if (input_data.my <= q_colors_screen_pos_y && input_data.my >= (q_colors_screen_pos_y - q_colors_screen_height))
				q_selected_cursor = &q_select_brush_cursor;

	static bool waiting_for_dt = false;

	if (dt_ready && waiting_for_dt)
	{
		graphics_update_quad_texture(&q_result, result_image, images_width, images_height);
		waiting_for_dt = false;
	}

	if (dt_ready && scribbles_added)
	{
		scribbles_added = false;
		lock_mutex(mutex);
		dt_ready = false;
		release_mutex(mutex);
		waiting_for_dt = true;
	}
}