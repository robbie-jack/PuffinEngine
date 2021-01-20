#version 460

layout(set = 0, binding = 0) uniform CameraData
{
	mat4 view;
	mat4 proj;
} camera;

layout(location = 0) in vec3 inPos;
layout(location = 2) in vec3 inColor;

layout(location = 0) out vec3 fragPos;
layout(location = 2) out vec4 fragColor;

void main()
{
	mat4 viewMatrix = camera.view;
	mat4 projMatrix = camera.proj;

	fragPos = inPos;
	fragColor = vec4(inColor, 1.0);

	gl_Position = projMatrix * viewMatrix * vec4(fragPos, 1.0);
}