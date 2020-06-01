#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) uniform LightBufferObject
{
	vec3 position;
	vec3 ambientColor;
	vec3 diffuseColor;
	float specularStrength;
	int shininess;
} light;

layout(binding = 2) uniform ViewBufferObject
{
	vec3 viewPos;
} camera;

layout(binding = 3) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec4 fragColor;
layout(location = 3) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() 
{
	vec3 lightDir = normalize(light.position - fragPosition);
	vec3 viewDir = normalize(camera.viewPos - fragPosition);
	vec3 halfwayDir = normalize(lightDir + viewDir);

	float diff = max(dot(fragNormal, lightDir), 0.0);
	vec3 diffuse = diff * light.diffuseColor;

	float spec = pow(max(dot(fragNormal, halfwayDir), 0.0), light.shininess);
	vec3 specular = light.specularStrength * spec * light.diffuseColor;

	vec3 result = (light.ambientColor + diffuse + specular) * fragColor.rgb;

    outColor = vec4(result * texture(texSampler, fragTexCoord).rgb, fragColor.a);
}