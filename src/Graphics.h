#pragma once
#include "Common.h"
#include "Mathematics.h"

typedef struct Quad_Struct Quad;

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

extern void graphics_quad_create(Vec4 position, Vec3 scale, Vec4 color, Quad* result);
extern void graphics_quad_create(Vec4 position, Vec3 scale, const u8* texture_path, Quad* result);
extern void graphics_quad_render(Quad* quad, u32 shader);
extern u32 graphics_shader_create(s8* vertex_shader_code, s8* fragment_shader_code);