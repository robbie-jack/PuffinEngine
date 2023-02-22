#version 460

layout (location = 0) in vec3 vPosition;
layout (location = 0) in vec3 vNormal;
layout (location = 0) in vec3 vTangent;
layout (location = 0) in vec2 vTexCoords;

layout (location = 0) out vec3 fColor;

layout(set = 0, binding = 0) uniform CameraBuffer
{
	mat4 view;
	mat4 proj;
	mat4 viewProj;
} cameraData;

void main()
{
	
}