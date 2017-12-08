#version 330 core

layout (location = 0) in vec4 vertex_position;
layout (location = 1) in vec2 vertex_tex_coords;

out vec2 fragment_tex_coords;

uniform mat4 model_matrix;

void main()
{
	gl_Position = model_matrix * vertex_position;
	fragment_tex_coords = vertex_tex_coords;
}