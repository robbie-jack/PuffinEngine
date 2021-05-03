#version 460

layout(set = 0, binding = 0) uniform CameraData
{
	mat4 viewProj;
} camera;

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec2 inUV;

layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec4 fragColor;

void main()
{
	fragPos = inPos;
	fragColor = vec4(inColor, 1.0);

	gl_Position = camera.viewProj * vec4(fragPos, 1.0);
}