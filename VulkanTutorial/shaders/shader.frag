#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform ViewBufferObject
{
	vec3 viewPos;
} camera;

struct LightData
{
	vec3 position;
	vec3 direction;

	vec3 ambientColor;
	vec3 diffuseColor;

	float cutoff;

	float constant;
	float linear;
	float quadratic;

	float specularStrength;
	int shininess;
};

layout(set = 0, binding = 1) uniform Light
{
	LightData data;
} light;

layout(set = 2, binding = 2) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec4 fragColor;
layout(location = 3) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

vec3 CalcPointLight(LightData light, vec3 normal, vec3 viewDir, vec3 fragPos);
vec3 CalcDirLight(LightData light, vec3 normal, vec3 viewDir);
vec3 CalcSpotLight(LightData light, vec3 normal, vec3 viewDir, vec3 fragPos);

void main() 
{
	vec3 viewDir = normalize(camera.viewPos - fragPosition);

	vec3 result = vec3(0.0, 0.0, 0.0);
	result = CalcDirLight(light.data, fragNormal, viewDir);
	result = CalcPointLight(light.data, fragNormal, viewDir, fragPosition);

    outColor = vec4(result * texture(texSampler, fragTexCoord).rgb, fragColor.a);
}

vec3 CalcPointLight(LightData light, vec3 normal, vec3 viewDir, vec3 fragPos)
{
	vec3 lightDir = normalize(light.position - fragPos);
	vec3 halfwayDir = normalize(lightDir + viewDir);

	float distance = length(light.position - fragPos);
	float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

	vec3 ambient = light.ambientColor * attenuation;

	float diff = max(dot(normal, lightDir), 0.0);
	vec3 diffuse = diff * light.diffuseColor * attenuation;

	float spec = pow(max(dot(normal, halfwayDir), 0.0), light.shininess);
	vec3 specular = light.specularStrength * spec * light.diffuseColor * attenuation;
	
	return ambient + diffuse + specular;
}

vec3 CalcDirLight(LightData light, vec3 normal, vec3 viewDir)
{
	vec3 lightDir = normalize(-light.direction);
	vec3 halfwayDir = normalize(lightDir + viewDir);

	float diff = max(dot(normal, lightDir), 0.0);
	vec3 diffuse = diff * light.diffuseColor;

	float spec = pow(max(dot(normal, halfwayDir), 0.0), light.shininess);
	vec3 specular = light.specularStrength * spec * light.diffuseColor;
	
	return light.ambientColor + diffuse + specular;return vec3(0.0, 0.0, 0.0);
}

vec3 CalcSpotLight(LightData light, vec3 normal, vec3 viewDir, vec3 fragPos)
{
	return vec3(0.0, 0.0, 0.0);
}