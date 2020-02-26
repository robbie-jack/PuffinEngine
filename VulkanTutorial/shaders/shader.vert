#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject
{
	mat4 model;
	mat4 view;
	mat4 proj;
} ubo;

layout(binding = 1) uniform LightBufferObject
{
	vec3 position;
	vec3 ambientColor;
	vec3 diffuseColor;
} light;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec3 fragPosition;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec4 fragColor;
layout(location = 3) out vec2 fragTexCoord;

void main() 
{
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);

	float ambientStrength = 0.1;
	vec3 ambient = ambientStrength * light.ambientColor;

	fragPosition = vec3(ubo.model * vec4(inPosition, 1.0));

	vec3 inverse_normal = vec3(inverse(ubo.model) * vec4(inNormal, 1.0));

	vec3 norm = normalize(inverse_normal);
	vec3 lightDir = normalize(light.position - fragPosition);

	float diff = max(dot(norm, lightDir), 0.0);
	vec3 diffuse = diff * light.diffuseColor;

	vec3 result = (ambient + diffuse) * inColor;

	fragNormal = inNormal;
    fragColor = vec4(result, 1.0);
	fragTexCoord = inTexCoord;
}