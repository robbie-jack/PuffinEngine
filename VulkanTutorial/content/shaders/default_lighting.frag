#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : enable

layout(set = 3, binding = 0) uniform ViewBufferObject
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

	int shadowmapIndex;
};

struct DirectionalLightData
{
	vec3 ambientColor;
	vec3 diffuseColor;

	vec3 direction;

	float specularStrength;
	int shininess;

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

	int shadowmapIndex;
};

layout(std140, set = 4, binding = 0) readonly buffer PointLightBuffer
{
	PointLightData lights[];
} pointBuffer;

layout(std140, set = 4, binding = 1) readonly buffer DirectionalLightBuffer
{
	DirectionalLightData lights[];
} directionalBuffer;

layout(std140, set = 4, binding = 2) readonly buffer SpotLightBuffer
{
	SpotLightData lights[];
} spotBuffer;

layout(set = 4, binding = 3) uniform LightStatsData
{
	int numPLights;
	int numDLights;
	int numSLights;
} lightStats;

layout(set = 5, binding = 0) uniform sampler2D shadowmaps[];

layout(set = 6, binding = 0) uniform sampler2D texSampler;

const int maxLights = 12;

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec4 fragColor;
layout(location = 3) in vec2 fragUV;
layout(location = 4) in vec4 fragPosLightSpace[maxLights];

layout(location = 0) out vec4 outColor;

vec3 CalcPointLight(PointLightData light, vec3 normal, vec3 viewDir, vec3 fragPos);
vec3 CalcDirLight(DirectionalLightData light, vec3 normal, vec3 viewDir);
vec3 CalcSpotLight(SpotLightData light, vec3 normal, vec3 viewDir, vec3 fragPos);

float ShadowCalculation(sampler2D shadowMap, vec4 fragPosLightSpace, vec3 normal, vec3 lightDir);

void main() 
{
	vec3 viewDir = normalize(camera.viewPos - fragPos);

	vec3 result = vec3(0.0, 0.0, 0.0);

	// Calculate Point Lights
	for (int i = 0; i < lightStats.numPLights; i++)
	{
		result += CalcPointLight(pointBuffer.lights[i], fragNormal, viewDir, fragPos);
	}

	// Calculate Directional Lights
	for (int i = 0; i < lightStats.numDLights; i++)
	{
		result += CalcDirLight(directionalBuffer.lights[i], fragNormal, viewDir);
	}

	// Calculate Spot Lights
	for (int i = 0; i < lightStats.numSLights; i++)
	{
		result += CalcSpotLight(spotBuffer.lights[i], fragNormal, viewDir, fragPos);
	}

    outColor = vec4(result * texture(texSampler, fragUV).rgb, fragColor.a);
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

	float shadow = light.shadowmapIndex != -1 ? 
	ShadowCalculation(shadowmaps[light.shadowmapIndex], fragPosLightSpace[light.shadowmapIndex], normal, lightDir) : 1.0;

	diffuse *= intensity;
	specular *= intensity;

	return ambient + (shadow * (diffuse + specular));
}

float ShadowCalculation(sampler2D shadowMap, vec4 fragPosLightSpace, vec3 normal, vec3 lightDir)
{
	// Get Projected Shadow Coordinates
	vec3 shadowCoord = fragPosLightSpace.xyz / fragPosLightSpace.w;

	// Sample closest depth value from shadowmap
	float closestDepth = texture(shadowMap, shadowCoord.xy).r;

	float bias = 0.05;
	//float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);

	// Calculate depth value of current pixel;
	float currentDepth = shadowCoord.z - bias;

	// Check if current fragment is in shadow
	float shadow = closestDepth < currentDepth ? 0.0 : 1.0;

	// Depth Values farther than lights far plane should return shadow value of 1
	shadow =  currentDepth > 1.0 ? 1.0 : shadow;

	return shadow;
}