#include "Graphics.h"
#include "Util.h"
#include <stb_image.h>
#include <glad.h>

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
		{ -1.0f, -1.0f, +0.0f, +1.0f, 0.0f, 0.0f },
		{ +1.0f, -1.0f, +0.0f, +1.0f, 1.0f, 0.0f },
		{ -1.0f, +1.0f, +0.0f, +1.0f, 0.0f, 1.0f },
		{ +1.0f, +1.0f, +0.0f, +1.0f, 1.0f, 1.0f }
	};

	s32 indices[2][3] = {
		{ 0, 1, 2 },
		{ 1, 2, 3 }
	};

	GLuint VAO = createVAO((r32*)vertices, sizeof(vertices), (s32*)indices, sizeof(indices));

	result->position = position;
	result->scale = scale;
	result->VAO = VAO;
	result->color = color;
	result->use_texture = false;
}

extern void graphics_quad_create(Vec4 position, Vec3 scale, const u8* texture_path, Quad* result)
{
	// 4-position, 2-texcoords
	r32 vertices[4][6] = {
		{ -1.0f, -1.0f, +0.0f, +1.0f, 0.0f, 0.0f },
		{ +1.0f, -1.0f, +0.0f, +1.0f, 1.0f, 0.0f },
		{ -1.0f, +1.0f, +0.0f, +1.0f, 0.0f, 1.0f },
		{ +1.0f, +1.0f, +0.0f, +1.0f, 1.0f, 1.0f }
	};

	s32 indices[2][3] = {
		{ 0, 1, 2 },
		{ 1, 2, 3 }
	};

	GLuint VAO = createVAO((r32*)vertices, sizeof(vertices), (s32*)indices, sizeof(indices));

	stbi_set_flip_vertically_on_load(1);
	int image_width, image_height, image_channels;
	unsigned char* image_data = stbi_load((const s8*)texture_path, &image_width, &image_height, &image_channels, 4);
	GLuint tex_id;

	glGenTextures(1, &tex_id);
	glBindTexture(GL_TEXTURE_2D, tex_id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glBindTexture(GL_TEXTURE_2D, 0);

	stbi_image_free(image_data);

	result->position = position;
	result->scale = scale;
	result->VAO = VAO;
	result->texture_id = tex_id;
	result->use_texture = true;
}

extern void graphics_quad_render(Quad* quad, u32 shader)
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

	glBindVertexArray(quad->VAO);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);

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