#version 460
#extension GL_ARB_seperate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : enable

layout(set = 0, binding = 0) uniform UBO
{
	vec3 viewPos;
	int displayDebugTarget;
} ubo;

layout(set = 0, binding = 1) uniform sampler2D samplerPosition;
layout(set = 0, binding = 2) uniform sampler2D samplerNormal;
layout(set = 0, binding = 3) uniform sampler2D samplerAlbedoSpec;

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

layout(std140, set = 0, binding = 4) readonly buffer PointLightBuffer
{
	PointLightData data[];
} pointLights;

layout(std140, set = 0, binding = 5) readonly buffer DirectionalLightBuffer
{
	DirectionalLightData data[];
} dirLights;

layout(std140, set = 0, binding = 6) readonly buffer SpotLightBuffer
{
	SpotLightData data[];
} spotLights;

layout(set = 0, binding = 7) uniform LightStatsData
{
	int numPointLights;
	int numDirLights;
	int numSpotLights;
} lightStats;

layout(set = 1, binding = 0) uniform sampler2D shadowmaps[];

layout(location = 0) in vec2 fragUV;

layout (location = 0) out vec4 outColor;

// Calculate how much light is contributed to fragment
vec3 CalcPointLight(PointLightData light, vec3 normal, vec3 viewDir, vec3 fragPos);
vec3 CalcDirLight(DirectionalLightData light, vec3 normal, vec3 viewDir);
vec3 CalcSpotLight(SpotLightData light, vec3 normal, vec3 viewDir, vec3 fragPos);

// Calculate if fragment is in shadow
float ShadowCalculation(sampler2D shadowMap, vec4 fragPosLightSpace, vec3 normal, vec3 lightDir);

void main()
{
	// Get G-Buffer Values
	vec3 fragPos = texture(samplerPosition, fragUV).rgb;
	vec3 fragNormal = texture(samplerNormal, fragUV).rgb;
	vec4 fragAlbedoSpec = texture(samplerAlbedoSpec, fragUV);

	// Render G-Buffer directly to Screen
	if (ubo.displayDebugTarget > 0)
	{
		switch (ubo.displayDebugTarget)
		{
			case 1: 
				outColor.rgb = fragPos;
				break;
			case 2: 
				outColor.rgb = fragNormal;
				break;
			case 3: 
				outColor.rgb = fragAlbedoSpec.rgb;
				break;
			case 4: 
				outColor.rgb = fragAlbedoSpec.aaa;
				break;
		}
		outColor.a = 1.0;
		return;
	}

	vec3 viewDir = normalize(ubo.viewPos - fragPos);

	vec3 result = vec3(0.0, 0.0, 0.0);

	// Calculate Point Lights
	int pLights = lightStats.numPointLights;
	for (int i = 0; i < pLights; i++)
	{
		result += CalcPointLight(pointLights.data[i], fragNormal, viewDir, fragPos);
	}

	// Calculate Directional Lights
	int dLights = lightStats.numDirLights;
	for (int i = 0; i < dLights; i++)
	{
		result += CalcDirLight(dirLights.data[i], fragNormal, viewDir);
	}

	// Calculate Spot Lights
	int sLights = lightStats.numSpotLights;
	for (int i = 0; i < sLights; i++)
	{
		result += CalcSpotLight(spotLights.data[i], fragNormal, viewDir, fragPos);
	}

	outColor = vec4(fragAlbedoSpec.rgb * result, 1.0);
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

	//float shadow = light.shadowmapIndex != -1 ? 
	//ShadowCalculation(shadowmaps[light.shadowmapIndex], fragPosLightSpace[light.shadowmapIndex], normal, lightDir) : 1.0;

	diffuse *= intensity;
	specular *= intensity;

	//return ambient + (shadow * (diffuse + specular));
	return ambient + diffuse + specular;
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