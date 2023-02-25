#version 460

layout (location = 0) in vec3 fPos;
layout (location = 1) in vec3 fNormal;
layout (location = 2) in vec3 fTangent;
layout (location = 3) in vec2 fUV;
layout (location = 4) flat in int texIndex;

layout (location = 0) out vec4 outColor;

void main()
{
	outColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
}