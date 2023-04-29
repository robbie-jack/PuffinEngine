#version 460

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec3 vTangent;
layout (location = 3) in vec2 vUV;

layout (location = 0) out vec4 fWorldPos;
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
	int texIndex;
};

layout(std140, set = 0, binding = 1) readonly buffer ObjectBuffer
{
	ObjectData objects[];
} objectBuffer;

void main()
{
	mat4 modelMatrix = objectBuffer.objects[gl_BaseInstance].model;
	mat4 modelMatrixInv = inverse(modelMatrix);
	mat4 viewProjMatrix = cameraData.viewProj;
		
	fWorldPos = modelMatrix * vec4(vPosition, 1.0f);
	fUV = vUV;

	mat3 mNormal = transpose(mat3(modelMatrixInv));
	fNormal = normalize(mNormal * vNormal);
	fTangent = normalize(mNormal * vTangent);
	//fNormal = (viewProjMatrix * vec4(vNormal, 1.0)).rgb;
	//fTangent = (viewProjMatrix * vec4(vTangent, 1.0)).rgb;
	
	texIndex = objectBuffer.objects[gl_BaseInstance].texIndex;

	gl_Position = viewProjMatrix * fWorldPos;
}