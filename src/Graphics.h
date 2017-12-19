#pragma once
#include "Common.h"
#include "Mathematics.h"

typedef struct Quad_Struct Quad;
typedef struct Virtual_Window_Struct Virtual_Window;
typedef struct Text_Struct Text;
typedef void (Virtual_Window_Render_Function(Virtual_Window*));
typedef void (Virtual_Window_Update_Function(Virtual_Window*));

struct Text_Struct
{
	r32 x;
	r32 y;
	r32 width;
	r32 height;
	s32 text_VAO;
};

struct Quad_Struct
{
	Vec4 position;
	Vec3 scale;
	u32 VAO;
	s32 use_texture;
	
	union
	{
		Vec4 color;
		u32 texture_id;
	};
};

struct Virtual_Window_Struct
{
	r32 x;
	r32 y;
	r32 width;
	r32 height;
	s32 being_dragged;
	Vec4 drag_position;
	Virtual_Window_Render_Function* render_function;
	Virtual_Window_Update_Function* update_function;
	Quad drawable_quad;
	Quad title_quad;
	bool title_has_text;
	Text title_text;
};

extern u32 quad_shader;

extern void graphics_init();
extern u32 graphics_shader_create(s8* vertex_shader_code, s8* fragment_shader_code);
extern void graphics_quad_create(Vec4 position, Vec3 scale, Vec4 color, Quad* result);
extern void graphics_quad_create(Vec4 position, Vec3 scale, const r32* image_data, s32 image_width, s32 image_height, Quad* result);
extern void graphics_quad_render(Quad* quad, u32 shader, u32 mode);
extern Virtual_Window* graphics_virtual_window_create(r32 x, r32 y, r32 width, r32 height,
	Virtual_Window_Render_Function render_function, Virtual_Window_Update_Function update_function, bool title_has_text, s32 title_number);
extern void graphics_virtual_windows_render();
extern void graphics_virtual_windows_update();
extern bool graphics_can_virtual_window_process_input(Virtual_Window* vw);
extern void graphics_update_quad_texture(Quad* quad, const r32* image_data, s32 image_width, s32 image_height);
extern Text graphics_text_create(s32 text_number, r32 x, r32 y, r32 width, r32 height);
extern void graphics_text_render(Text* t, u32 shader);