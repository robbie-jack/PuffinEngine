#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(set = 2, binding = 0) uniform ViewBufferObject
{
	vec3 viewPos;
} camera;

struct PointLightData
{
	vec3 ambientColor;
	vec3 diffuseColor;

	vec3 position;

	float constant;
	float linear;
	float quadratic;

	float specularStrength;
	int shininess;

	mat4 lightSpaceMatrix;
	int shadowmapIndex;
};

struct DirectionalLightData
{
	vec3 ambientColor;
	vec3 diffuseColor;

	vec3 direction;

	float specularStrength;
	int shininess;

	mat4 lightSpaceMatrix;
	int shadowmapIndex;
};

struct SpotLightData
{
	vec3 ambientColor;
	vec3 diffuseColor;

	vec3 position;
	vec3 direction;

	float innerCutoff;
	float outerCutoff;

	float constant;
	float linear;
	float quadratic;

	float specularStrength;
	int shininess;

	mat4 lightSpaceMatrix;
	int shadowmapIndex;
};

layout(std140, set = 3, binding = 0) readonly buffer PointLightBuffer
{
	PointLightData lights[];
} pointBuffer;

layout(std140, set = 3, binding = 1) readonly buffer DirectionalLightBuffer
{
	DirectionalLightData lights[];
} directionalBuffer;

layout(std140, set = 3, binding = 2) readonly buffer SpotLightBuffer
{
	SpotLightData lights[];
} spotBuffer;

layout(set = 3, binding = 3) uniform LightStatsData
{
	int numPLights;
	int numDLights;
	int numSLights;
} lightStats;

//layout(set = 4, binding = 0) uniform sampler2D shadowmaps[];

layout(set = 4, binding = 0) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec4 fragColor;
layout(location = 3) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

vec3 CalcPointLight(PointLightData light, vec3 normal, vec3 viewDir, vec3 fragPos);
vec3 CalcDirLight(DirectionalLightData light, vec3 normal, vec3 viewDir);
vec3 CalcSpotLight(SpotLightData light, vec3 normal, vec3 viewDir, vec3 fragPos);

float ShadowCalculation(vec4 fragPosLightSpace);

void main() 
{
	vec3 viewDir = normalize(camera.viewPos - fragPosition);

	vec3 result = vec3(0.0, 0.0, 0.0);
	//result = CalcDirLight(light.data, fragNormal, viewDir);
	//result = CalcPointLight(light.data, fragNormal, viewDir, fragPosition);
	//result = CalcSpotLight(light.data, fragNormal, viewDir, fragPosition);

	// Calculate Point Lights
	for (int i = 0; i < lightStats.numPLights; i++)
	{
		result += CalcPointLight(pointBuffer.lights[i], fragNormal, viewDir, fragPosition);
	}

	// Calculate Directional Lights
	for (int i = 0; i < lightStats.numDLights; i++)
	{
		result += CalcDirLight(directionalBuffer.lights[i], fragNormal, viewDir);
	}

	// Calculate Spot Lights
	for (int i = 0; i < lightStats.numSLights; i++)
	{
		result += CalcSpotLight(spotBuffer.lights[i], fragNormal, viewDir, fragPosition);
	}

	//result = CalcSpotLight(spotBuffer.lights[0], fragNormal, viewDir, fragPosition);

    outColor = vec4(result * texture(texSampler, fragTexCoord).rgb, fragColor.a);
}

vec3 CalcPointLight(PointLightData light, vec3 normal, vec3 viewDir, vec3 fragPos)
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

vec3 CalcDirLight(DirectionalLightData light, vec3 normal, vec3 viewDir)
{
	vec3 lightDir = normalize(-light.direction);
	vec3 halfwayDir = normalize(lightDir + viewDir);

	float diff = max(dot(normal, lightDir), 0.0);
	vec3 diffuse = diff * light.diffuseColor;

	float spec = pow(max(dot(normal, halfwayDir), 0.0), light.shininess);
	vec3 specular = light.specularStrength * spec * light.diffuseColor;
	
	return light.ambientColor + diffuse + specular;
}

vec3 CalcSpotLight(SpotLightData light, vec3 normal, vec3 viewDir, vec3 fragPos)
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

	vec4 fragPosLightSpace = light.lightSpaceMatrix * vec4(fragPosition, 1.0);

	float shadow = ShadowCalculation(fragPosLightSpace);

	diffuse *= intensity;
	specular *= intensity;

	return ambient + diffuse + specular;
}

float ShadowCalculation(vec4 fragPosLightSpace)
{
	return 1.0;
}