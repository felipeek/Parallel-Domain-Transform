#include "Graphics.h"
#include "Util.h"
#include "Input.h"
#include "OS.h"
#include <dynamic_array.h>
#include <glad.h>

#define VIRTUAL_WINDOW_TITLE_QUAD_COLOR { 0.015f, 0.03f, 0.4f, 0.4f }
#define VIRTUAL_WINDOW_DRAWBLE_QUAD_COLOR { 0.02f, 0.05f, 0.7f, 0.4f }
#define VIRTUAL_WINDOW_TITLE_HEIGHT 0.07f

Virtual_Window* virtual_windows;

// Shaders
GLuint quad_shader;

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

extern void graphics_init()
{
	load_shaders();
	virtual_windows = array_create(Virtual_Window, 8);
}

static GLuint createVAO(r32* vertices, s32 vertices_size, s32* indices, s32 indices_size)
{
	GLuint VAO, VBO, EBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, vertices_size, 0, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, vertices_size, vertices);

	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), 0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)(4 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_size, 0, GL_STATIC_DRAW);
	glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indices_size, indices);

	glBindVertexArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	return VAO;
}

extern void graphics_quad_create(Vec4 position, Vec3 scale, Vec4 color, Quad* result)
{
	// 4-position, 2-texcoords
	r32 vertices[4][6] = {
		{ +0.0f, +0.0f, +0.0f, +1.0f, 0.0f, 1.0f },
		{ +0.0f, -1.0f, +0.0f, +1.0f, 0.0f, 0.0f },
		{ +1.0f, +0.0f, +0.0f, +1.0f, 1.0f, 1.0f },
		{ +1.0f, -1.0f, +0.0f, +1.0f, 1.0f, 0.0f }
	};

	s32 indices[2][3] = {
		{ 0, 1, 2 },
		{ 1, 3, 2 }
	};

	GLuint VAO = createVAO((r32*)vertices, sizeof(vertices), (s32*)indices, sizeof(indices));

	result->position = position;
	result->scale = scale;
	result->VAO = VAO;
	result->color = color;
	result->use_texture = false;
}

extern void graphics_quad_create(Vec4 position, Vec3 scale, const r32* image_data, s32 image_width, s32 image_height, Quad* result)
{
	// 4-position, 2-texcoords
	r32 vertices[4][6] = {
		{ +0.0f, +0.0f, +0.0f, +1.0f, 0.0f, 1.0f },
		{ +0.0f, -1.0f, +0.0f, +1.0f, 0.0f, 0.0f },
		{ +1.0f, +0.0f, +0.0f, +1.0f, 1.0f, 1.0f },
		{ +1.0f, -1.0f, +0.0f, +1.0f, 1.0f, 0.0f }
	};

	s32 indices[2][3] = {
		{ 0, 1, 2 },
		{ 1, 3, 2 }
	};

	GLuint VAO = createVAO((r32*)vertices, sizeof(vertices), (s32*)indices, sizeof(indices));

	GLuint tex_id;

	glGenTextures(1, &tex_id);
	glBindTexture(GL_TEXTURE_2D, tex_id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, image_width, image_height, 0, GL_RGBA, GL_FLOAT, image_data);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glBindTexture(GL_TEXTURE_2D, 0);

	result->position = position;
	result->scale = scale;
	result->VAO = VAO;
	result->texture_id = tex_id;
	result->use_texture = true;
}

extern void graphics_quad_render(Quad* quad, u32 shader, u32 mode)
{
	Mat4 scale_matrix =
	{
		{
			{ quad->scale.x, 0.0f, 0.0f, 0.0f },
			{ 0.0f, quad->scale.y, 0.0f, 0.0f },
			{ 0.0f, 0.0f, quad->scale.z , 0.0f },
			{ 0.0f, 0.0f, 0.0f, 1.0f }
		}
	};

	Mat4 translation_matrix =
	{
		{
			{ 1.0f, 0.0f, 0.0f, quad->position.x },
			{ 0.0f, 1.0f, 0.0f, quad->position.y },
			{ 0.0f, 0.0f, 1.0f, quad->position.z },
			{ 0.0f, 0.0f, 0.0f, 1.0f },
		}
	};

	Mat4 model_matrix, transposed_model_matrix;

	mat4_multiply(&translation_matrix, &scale_matrix, &model_matrix);
	mat4_transpose(&model_matrix, &transposed_model_matrix);

	glUseProgram(shader);

	GLuint model_matrix_location = glGetUniformLocation(shader, "model_matrix");
	glUniformMatrix4fv(model_matrix_location, 1, GL_FALSE, (r32*)transposed_model_matrix.m);

	if (quad->use_texture)
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, quad->texture_id);
		GLuint bg_texture_location = glGetUniformLocation(shader, "bg_texture");
		glUniform1i(bg_texture_location, 0);
	}
	else
	{
		GLuint bg_color_location = glGetUniformLocation(shader, "bg_color");
		glUniform4f(bg_color_location, quad->color.x, quad->color.y, quad->color.z, quad->color.w);
	}

	GLuint bg_use_texture = glGetUniformLocation(shader, "bg_use_texture");
	glUniform1i(bg_use_texture, quad->use_texture);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glBindVertexArray(quad->VAO);
	glDrawElements(mode, 6, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);

	glDisable(GL_BLEND);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

extern GLuint graphics_shader_create(s8* vertex_shader_code, s8* fragment_shader_code)
{
	GLint success;
	GLchar infoLogBuffer[1024];
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(vertexShader, 1, &vertex_shader_code, 0);
	glShaderSource(fragmentShader, 1, &fragment_shader_code, 0);

	glCompileShader(vertexShader);
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertexShader, 1024, 0, infoLogBuffer);
		print("Error compiling vertex shader: %s\n", infoLogBuffer);
	}

	glCompileShader(fragmentShader);
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fragmentShader, 1024, 0, infoLogBuffer);
		print("Error compiling fragment shader: %s\n", infoLogBuffer);
	}

	GLuint program = glCreateProgram();

	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);
	glLinkProgram(program);

	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(program, 1024, 0, infoLogBuffer);
		print("Error linking program: %s\n", infoLogBuffer);
	}

	return program;
}

extern Virtual_Window* graphics_virtual_window_create(r32 x, r32 y, r32 width, r32 height, Virtual_Window_Render_Function render_function, Virtual_Window_Update_Function update_function)
{
	Virtual_Window virtual_window;
	virtual_window.x = x;
	virtual_window.y = y;
	virtual_window.width = width;
	virtual_window.height = height;
	virtual_window.render_function = render_function;
	virtual_window.update_function = update_function;
	virtual_window.being_dragged = 0;

	Vec4 title_position = { x, y, 0.0f, 1.0f };
	Vec3 title_scale = { width, VIRTUAL_WINDOW_TITLE_HEIGHT, 0.0f };
	Vec4 title_color = VIRTUAL_WINDOW_TITLE_QUAD_COLOR;

	graphics_quad_create(title_position, title_scale, title_color, &virtual_window.title_quad);

	Vec4 drawable_position = { x, y - VIRTUAL_WINDOW_TITLE_HEIGHT, 0.0f, 1.0f };
	Vec3 drawable_scale = { width, height - VIRTUAL_WINDOW_TITLE_HEIGHT, 0.0f };
	Vec4 drawable_color = VIRTUAL_WINDOW_DRAWBLE_QUAD_COLOR;

	graphics_quad_create(drawable_position, drawable_scale, drawable_color, &virtual_window.drawable_quad);

	array_push(virtual_windows, &virtual_window);

	return &virtual_windows[array_get_length(virtual_windows) - 1];
}

extern void graphics_virtual_windows_render()
{
	for (u32 i = 0; i < array_get_length(virtual_windows); ++i)
	{
		graphics_quad_render(&virtual_windows[i].drawable_quad, quad_shader, GL_TRIANGLES);
		virtual_windows[i].render_function(virtual_windows + i);
		graphics_quad_render(&virtual_windows[i].title_quad, quad_shader, GL_TRIANGLES);
	}
}

extern void graphics_virtual_windows_update()
{
	// Input Update
	for (u32 i = 0; i < array_get_length(virtual_windows); ++i)
	{
		Virtual_Window* vw = &virtual_windows[i];

		//print("Mouse Loc: <%f, %f>\n", input_data.mx, input_data.my);
		//print("Virtual Window Loc: <%f, %f> | <%f, %f>\n\n", vw->x, vw->y, vw->width, vw->height);

		if (vw->being_dragged && input_data.mclicked)
		{
			// Windows is being dragged and mouse lbutton is still pressed

			Vec2 drag_difference = { input_data.mx - vw->drag_position.x, input_data.my - vw->drag_position.y };
			vw->x += drag_difference.x;
			vw->y += drag_difference.y;
			vw->drag_position = { input_data.mx, input_data.my, 0.0f, 0.0f };
		}
		else if (vw->being_dragged && !input_data.mclicked)
		{
			// Windows is being dragged, but mouse lbutton is not pressed anymore
			vw->being_dragged = false;
		}
		else if (!vw->being_dragged && input_data.mclicked)
		{
			// Window is not being dragged, but mouse lbutton is pressed

			// Check if there is a dragged window
			bool is_there_a_dragged_window = false;
			for (u32 j = 0; j < array_get_length(virtual_windows); ++j)
			{
				Virtual_Window* _vw = &virtual_windows[j];
				if (_vw->being_dragged)
				{
					is_there_a_dragged_window = true;
					break;
				}
			}

			if (!is_there_a_dragged_window)
				if (input_data.mx >= vw->x && input_data.mx <= (vw->x + vw->width))
					if (input_data.my <= vw->y && input_data.my >= (vw->y - VIRTUAL_WINDOW_TITLE_HEIGHT))
					{
						vw->drag_position = { input_data.mx, input_data.my, 0.0f, 0.0f };
						vw->being_dragged = true;
					}
		}
	}

	// Virtual Windows Attributes Update
	for (u32 i = 0; i < array_get_length(virtual_windows); ++i)
	{
		Virtual_Window* vw = &virtual_windows[i];

		vw->title_quad.position = { vw->x, vw->y, 0.0f, 1.0f };
		vw->title_quad.scale = { vw->width, VIRTUAL_WINDOW_TITLE_HEIGHT, 0.0f };

		vw->drawable_quad.position = { vw->x, vw->y - VIRTUAL_WINDOW_TITLE_HEIGHT, 0.0f, 1.0f };
		vw->drawable_quad.scale = { vw->width, vw->height - VIRTUAL_WINDOW_TITLE_HEIGHT, 0.0f };
	}

	// Update window elements
	for (u32 i = 0; i < array_get_length(virtual_windows); ++i)
		virtual_windows[i].update_function(virtual_windows + i);
}

extern bool graphics_can_virtual_window_process_input(Virtual_Window* vw)
{
	// Check if there is a dragged window
	bool is_there_a_dragged_window = false;
	for (u32 j = 0; j < array_get_length(virtual_windows); ++j)
	{
		Virtual_Window* _vw = &virtual_windows[j];
		if (_vw->being_dragged)
		{
			is_there_a_dragged_window = true;
			break;
		}
	}

	if (!is_there_a_dragged_window)
		if (input_data.mx >= vw->x && input_data.mx <= (vw->x + vw->width))
			if (input_data.my <= (vw->y - VIRTUAL_WINDOW_TITLE_HEIGHT) && input_data.my >= (vw->y - VIRTUAL_WINDOW_TITLE_HEIGHT - vw->height))
				return true;

	return false;
}

extern void graphics_update_quad_texture(Quad* quad, const r32* image_data, s32 image_width, s32 image_height)
{
	glBindTexture(GL_TEXTURE_2D, quad->texture_id);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, image_width, image_height, GL_RGBA, GL_FLOAT, image_data);
	glBindTexture(GL_TEXTURE_2D, 0);
}