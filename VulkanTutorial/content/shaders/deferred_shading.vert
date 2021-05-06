#version 460
#extension GL_ARB_seperate_shader_objects : enable

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec2 inUV;

layout (location = 0) out vec2 fragUV;

void main()
{
	fragUV = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
	gl_Position = vec4(fragUV * 2.0f - 1.0f, 0.0f, 1.0f);
}