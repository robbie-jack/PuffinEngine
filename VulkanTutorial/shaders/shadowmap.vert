#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(std140, set = 0, binding = 0) readonly buffer LightBuffer
{
	mat4 lightSpaceMatrix[];
} lightBuffer;

layout(set = 0, binding = 1) uniform LightData
{
	int index;
} lightData;

struct ObjectData
{
	mat4 model;
	mat4 inv_model;
};

layout(std140, set = 1, binding = 0) readonly buffer ObjectBuffer
{
	ObjectData objects[];
} objectBuffer;

layout (location = 0) in vec3 inPos;

void main()
{
	mat4 modelMatrix = objectBuffer.objects[gl_BaseInstance].model;

	gl_Position = lightBuffer.lightSpaceMatrix[lightData.index] * modelMatrix * vec4(inPos, 1.0);
}