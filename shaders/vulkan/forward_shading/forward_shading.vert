#version 460

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec3 vTangent;
layout (location = 3) in vec2 vUV;

layout (location = 0) out vec3 fPos;
layout (location = 1) out vec3 fNormal;
layout (location = 2) out vec3 fTangent;
layout (location = 3) out vec2 fUV;
layout (location = 4) flat out int texIndex;

layout(set = 0, binding = 0) uniform CameraBuffer
{
	mat4 view;
	mat4 proj;
	mat4 viewProj;
} cameraData;

struct ObjectData
{
	mat4 model;
	mat4 invModel;
	int texIndex;
};

layout(std140, set = 0, binding = 1) readonly buffer ObjectBuffer
{

	ObjectData objects[];
} objectBuffer;

void main()
{
	mat4 modelMatrix = objectBuffer.objects[gl_BaseInstance].model;
	mat4 transformMatrix = (cameraData.viewProj * modelMatrix);
	
	gl_Position = transformMatrix * vec4(vPosition, 1.0f);
	
	fPos = gl_Position.xyz;
	fNormal = vNormal;
	fTangent = vTangent;
	fUV = vUV;
	
	texIndex = objectBuffer.objects[gl_BaseInstance].texIndex;
}