#version 460

layout(set = 0, binding = 0) uniform CameraData
{
	mat4 viewProj;
} camera;

struct ObjectUBO
{
	mat4 model;
	mat4 inv_model;
};

layout(std140, set = 0, binding = 1) readonly buffer ObjectBuffer
{
	ObjectUBO objects[];
} objectBuffer;

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec2 inUV;

layout(location = 0) out vec4 fragWorldPos;
layout(location = 1) out vec3 fragColor;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) out vec3 fragTangent;
layout(location = 4) out vec2 fragUV;
layout(location = 5) flat out int fragInstance;

void main()
{
	mat4 modelMatrix = objectBuffer.objects[gl_BaseInstance].model;
	mat4 modelMatrixInverse = objectBuffer.objects[gl_BaseInstance].inv_model;
	mat4 viewProjMatrix = camera.viewProj;

	fragWorldPos = modelMatrix * vec4(inPos, 1.0);
	fragUV = inUV;
	fragColor = inColor;

	mat3 mNormal = transpose(mat3(modelMatrixInverse));
	fragNormal = mNormal * normalize(inNormal);
	fragTangent = mNormal * normalize(inTangent);

	fragInstance = gl_BaseInstance;

	gl_Position = viewProjMatrix *  fragWorldPos;
}