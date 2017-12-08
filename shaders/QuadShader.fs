#version 330 core

in vec2 fragment_tex_coords;

out vec4 final_color;

uniform vec4 bg_color;
uniform sampler2D bg_texture;
uniform bool bg_use_texture;

void main()
{
	if (bg_use_texture)
		final_color = texture(bg_texture, fragment_tex_coords);
	else
		final_color = bg_color;
}