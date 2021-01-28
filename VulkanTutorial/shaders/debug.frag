#version 460

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec4 fragColor;

layout(location = 0) out vec4 outColor;

void main()
{
	outColor = fragColor;
}