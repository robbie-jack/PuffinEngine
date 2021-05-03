#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform LightBuffer
{
	mat4 lightSpaceMatrix;
} light;

struct ObjectData
{
	mat4 model;
	mat4 inv_model;
};

layout(std140, set = 1, binding = 0) readonly buffer ObjectBuffer
{
	ObjectData objects[];
} objectBuffer;

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec2 inUV;

void main()
{
	mat4 modelMatrix = objectBuffer.objects[gl_BaseInstance].model;

	gl_Position = light.lightSpaceMatrix * modelMatrix * vec4(inPos, 1.0);
}