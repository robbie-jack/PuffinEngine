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

	float innerCutoff;
	float outerCutoff;

	float constant;
	float linear;
	float quadratic;

	float specularStrength;
	int shininess;
};

layout(std140, set = 1, binding = 0) readonly buffer PointLightBuffer
{
	LightData lights[];
} pointBuffer;

layout(std140, set = 1, binding = 1) readonly buffer DirectionalLightBuffer
{
	LightData lights[];
} directionalBuffer;

layout(std140, set = 1, binding = 2) readonly buffer SpotLightBuffer
{
	LightData lights[];
} spotBuffer;

layout(set = 2, binding = 0) uniform sampler2D texSampler;

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
	//result = CalcDirLight(light.data, fragNormal, viewDir);
	//result = CalcPointLight(light.data, fragNormal, viewDir, fragPosition);
	//result = CalcSpotLight(light.data, fragNormal, viewDir, fragPosition);

	// Calculate Point Lights
	for (int i = 0; i < pointBuffer.lights.length(); i++)
	{
		result += CalcPointLight(pointBuffer.lights[i], fragNormal, viewDir, fragPosition);
	}

	// Calculate Directional Lights
	for (int i = 0; i < directionalBuffer.lights.length(); i++)
	{
		result += CalcDirLight(directionalBuffer.lights[i], fragNormal, viewDir);
	}

	// Calculate Spot Lights
	for (int i = 0; i < spotBuffer.lights.length(); i++)
	{
		result += CalcSpotLight(spotBuffer.lights[i], fragNormal, viewDir, fragPosition);
	}

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
	
	return light.ambientColor + diffuse + specular;
}

vec3 CalcSpotLight(LightData light, vec3 normal, vec3 viewDir, vec3 fragPos)
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

	float theta = dot(lightDir, normalize(-light.direction));
	float epsilon = light.innerCutoff - light.outerCutoff;
	float intensity = clamp((theta - light.outerCutoff) / epsilon, 0.0, 1.0);

	diffuse *= intensity;
	specular *= intensity;

	return ambient + diffuse + specular;
}