#include "UI.h"
#include "Common.h"
#include "Util.h"
#include "Mathematics.h"
#include "Graphics.h"
#include "Input.h"
#include <glad.h>
#include <stb_image.h>
#include "ParallelDomainTransform.h"

#define DEFAULT_CURSOR_SIZE 0.05f

#define DEFAULT_CURSOR_PATH "./res/cursor/arrow-pointer.png"
#define SELECT_BRUSH_CURSOR_PATH "./res/cursor/map-pin.png"
#define PAINT_CURSOR_PATH "./res/cursor/pencil-cursor.png"

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

Virtual_Window* vw_result;
Virtual_Window* vw_scribbles;
Virtual_Window* vw_menu;

r32* scribbles;
r32* original_image;
r32* result_image;
s32 images_width;
s32 images_height;

Quad q_result;
Quad q_original_image;
Quad q_scribbles;
Quad q_colors[Q_COLORS_LINES][Q_COLORS_COLUMNS];
Quad q_selected_color;

Quad q_default_cursor;
Quad q_select_brush_cursor;
Quad q_paint_cursor;

Quad* q_selected_cursor;

//static void test()
//{
//	parallel_colorization(original_image, scribbles, )
//}

static void load_shaders()
{
}

static void create_image_quads()
{
	Vec4 position = { 0.0f, 0.0f, 0.0f, 1.0f };
	Vec3 scale = { 1.0f, 1.0f, 0.0f };

	stbi_set_flip_vertically_on_load(1);
	int image_channels;

	original_image = load_image("./res/fallen.png", &images_width, &images_height, &image_channels, 4);
	graphics_quad_create(position, scale, original_image, images_width, images_height, &q_result);

	result_image = load_image("./res/fallen.png", &images_width, &images_height, &image_channels, 4);
	graphics_quad_create(position, scale, result_image, images_width, images_height, &q_original_image);

	scribbles = (r32*)alloc_memory(images_width * images_height * image_channels * sizeof(r32));
	memory_set(scribbles, 0, images_width * images_height * image_channels * sizeof(r32));

	graphics_quad_create(position, scale, scribbles, images_width, images_height, &q_scribbles);
}

static void create_cursor_quads()
{
	stbi_set_flip_vertically_on_load(1);
	int image_width, image_height, image_channels;

	r32* image_data = load_image(DEFAULT_CURSOR_PATH, &image_width, &image_height, &image_channels, 4);
	graphics_quad_create({ 0.0f, 0.0f, 0.0f, 1.0f }, { DEFAULT_CURSOR_SIZE, DEFAULT_CURSOR_SIZE, DEFAULT_CURSOR_SIZE }, image_data,
		image_width, image_height, &q_default_cursor);
	stbi_image_free(image_data);

	image_data = load_image(SELECT_BRUSH_CURSOR_PATH, &image_width, &image_height, &image_channels, 4);
	graphics_quad_create({ 0.0f, 0.0f, 0.0f, 1.0f }, { DEFAULT_CURSOR_SIZE, DEFAULT_CURSOR_SIZE, DEFAULT_CURSOR_SIZE }, image_data,
		image_width, image_height, &q_select_brush_cursor);
	stbi_image_free(image_data);

	image_data = load_image(PAINT_CURSOR_PATH, &image_width, &image_height, &image_channels, 4);
	graphics_quad_create({ 0.0f, 0.0f, 0.0f, 1.0f }, { DEFAULT_CURSOR_SIZE, DEFAULT_CURSOR_SIZE, DEFAULT_CURSOR_SIZE }, image_data,
		image_width, image_height, &q_paint_cursor);
	stbi_image_free(image_data);

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

	for (s32 i = 0; i < Q_COLORS_LINES; ++i)
		for (s32 j = 0; j < Q_COLORS_COLUMNS; ++j)
		{
			position.x =  q_colors_screen_pos_x + j * (q_colors_screen_width / Q_COLORS_COLUMNS);
			position.y =  q_colors_screen_pos_y - i * (q_colors_screen_height / Q_COLORS_LINES);
			position.z = 0.0f;
			position.w = 1.0f;

			scale.x = q_colors_screen_width / Q_COLORS_COLUMNS;
			scale.y = q_colors_screen_height / Q_COLORS_LINES;
			scale.z = 1.0f;

			q_colors[i][j].position = position;
			q_colors[i][j].scale = scale;

			graphics_quad_render(&q_colors[i][j], quad_shader, GL_TRIANGLES);
		}

	position.x = vw->drawable_quad.position.x + Q_SELECTED_COLOR_X * vw->drawable_quad.scale.x;
	position.y = vw->drawable_quad.position.y - Q_SELECTED_COLOR_Y * vw->drawable_quad.scale.y;
	position.z = 0.0f;
	position.w = 1.0f;
	scale = { vw->drawable_quad.scale.x * Q_SELECTED_COLOR_WIDTH, vw->drawable_quad.scale.y * Q_SELECTED_COLOR_HEIGHT, 1.0f };
	q_selected_color.position = position;
	q_selected_color.scale = scale;
	graphics_quad_render(&q_selected_color, quad_shader, GL_TRIANGLES);
}

static void render_menu(Virtual_Window* vw)
{
	Vec4 position;
	Vec3 scale;

	for (s32 i = 0; i < Q_COLORS_LINES; ++i)
		for (s32 j = 0; j < Q_COLORS_COLUMNS; ++j)
			graphics_quad_render(&q_colors[i][j], quad_shader, GL_TRIANGLES);

	graphics_quad_render(&q_selected_color, quad_shader, GL_TRIANGLES);
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

		if (input_data.mclicked && graphics_can_virtual_window_process_input(vw))
		{
			r32 image_x_click = (input_data.mx - vw->drawable_quad.position.x) / vw->drawable_quad.scale.x;
			r32 image_y_click = (input_data.my - vw->drawable_quad.position.y + vw->drawable_quad.scale.y) / vw->drawable_quad.scale.y;

			s32 selected_x_pixel = r32_round(image_x_click * images_width);
			s32 selected_y_pixel = r32_round(image_y_click * images_height);

			scribbles[selected_y_pixel * images_width * 4 + selected_x_pixel * 4] = q_selected_color.color.x;
			scribbles[selected_y_pixel * images_width * 4 + selected_x_pixel * 4 + 1] = q_selected_color.color.y;
			scribbles[selected_y_pixel * images_width * 4 + selected_x_pixel * 4 + 2] = q_selected_color.color.z;
			scribbles[selected_y_pixel * images_width * 4 + selected_x_pixel * 4 + 3] = 1.0f;

			graphics_update_quad_texture(&q_scribbles, scribbles, images_width, images_height);

			//test();

			//print("image_x_click: %.2f\nimage_y_click: %.2f\n", image_x_click, image_y_click);
		}
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
	graphics_init();
	create_color_quads();
	create_cursor_quads();
	create_image_quads();

	vw_result = graphics_virtual_window_create(-1.0f, 1.0f, 1.0f, 1.0f, render_image_windows, update_image_windows);
	vw_scribbles = graphics_virtual_window_create(0.0f, 1.0f, 1.0f, 1.0f, render_image_windows, update_image_windows);
	vw_menu = graphics_virtual_window_create(-1.0f, -0.6f, 2.0f, 0.4f, render_menu, update_menu);

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
}