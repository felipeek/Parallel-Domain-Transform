#include "UI.h"
#include "Common.h"
#include "Util.h"
#include "Mathematics.h"
#include "Graphics.h"
#include <glad.h>

// Shaders
GLuint quad_shader;

// Temporary
Quad p_quad;

static void load_shaders()
{
	// Load basic shader
	static u8 vertex_shader_path[] = "./shaders/QuadShader.vs";
	static u8* vertex_shader_code = read_file(vertex_shader_path, 0);

	static u8 fragment_shader_path[] = "./shaders/QuadShader.fs";
	static u8* fragment_shader_code = read_file(fragment_shader_path, 0);

	quad_shader = graphics_shader_create((s8*)vertex_shader_code, (s8*)fragment_shader_code);

	free_file(vertex_shader_code);
	free_file(fragment_shader_code);
}

extern void ui_init()
{
	Vec4 quad_position = { 0.0f, 0.0f, 0.0f, 1.0f };
	Vec3 quad_scale = { 0.3f, 0.3f, 0.3f };
	u8 quad_texture[] = "./res/paint.png";
	graphics_quad_create(quad_position, quad_scale, quad_texture, &p_quad);

	load_shaders();
	glClearColor(0.5f, 0.3f, 0.0f, 1.0f);
}

extern void ui_render()
{
	glClear(GL_COLOR_BUFFER_BIT);
	graphics_quad_render(&p_quad, quad_shader);
}