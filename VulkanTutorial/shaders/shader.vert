#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform CameraData
{
	mat4 view;
	mat4 proj;
} camera;

struct ObjectData
{
	mat4 model;
	mat4 inv_model;
};

layout(std140, set = 1, binding = 0) readonly buffer ObjectBuffer
{
	ObjectData objects[];
} objectBuffer;

layout(std140, set = 2, binding = 0) readonly buffer LightBuffer
{
	mat4 lightSpaceMatrix[];
} lightBuffer;

layout(set = 2, binding = 1) uniform LightData
{
	int numLights;
} lightData;

const int maxLights = 12;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec3 fragPosition;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec4 fragColor;
layout(location = 3) out vec2 fragTexCoord;
layout(location = 4) out vec4 fragPosLightSpace[maxLights];

void main() 
{
	mat4 modelMatrix = objectBuffer.objects[gl_BaseInstance].model;
	mat4 inv_modelMatrix = objectBuffer.objects[gl_BaseInstance].inv_model;
	mat4 viewMatrix = camera.view;
	mat4 projMatrix = camera.proj;

	fragPosition = vec3(modelMatrix * vec4(inPosition, 1.0));
	fragNormal = normalize(mat3(transpose(inv_modelMatrix)) * inNormal);
    fragColor = vec4(inColor, 1.0);
	fragTexCoord = inTexCoord;

	for (int i = 0; i < lightData.numLights; i++)
	{
		fragPosLightSpace[i] = lightBuffer.lightSpaceMatrix[i] * modelMatrix * vec4(inPosition, 1.0);
	}

	gl_Position = projMatrix * viewMatrix * vec4(fragPosition, 1.0);
}