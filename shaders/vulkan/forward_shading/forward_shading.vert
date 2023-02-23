#version 460

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec3 vTangent;
layout (location = 3) in vec2 vTexCoords;

layout (location = 0) out vec3 fColor;

layout(set = 0, binding = 0) uniform CameraBuffer
{
	mat4 view;
	mat4 proj;
	mat4 viewProj;
} cameraData;

struct ObjectData
{
	mat4 model;
};

layout(std140, set = 1, binding = 0) readonly buffer ObjectBuffer
{

	ObjectData objects[];
} objectBuffer;

void main()
{
	mat4 modelMatrix = objectBuffer.objects[gl_BaseInstance].model;
	mat4 transformMatrix = (cameraData.viewProj * modelMatrix);
	
	gl_Position = transformMatrix * vec4(vPosition, 1.0f);
	
	fColor = vec3(1.0f, 1.0f, 1.0f);
}