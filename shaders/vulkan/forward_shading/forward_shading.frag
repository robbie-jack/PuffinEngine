#version 460
//#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : enable

layout (location = 0) in vec4 fWorldPos;
layout (location = 1) in vec3 fNormal;
layout (location = 2) in vec3 fTangent;
layout (location = 3) in vec2 fUV;
layout (location = 4) flat in int matIndex;

layout (location = 0) out vec4 outColor;

struct PointLightData
{
	vec4 positionShadowIndex;
	vec4 color;
	vec4 ambientSpecularExponent;
	vec4 attenuation;
};

struct SpotLightData
{
	vec4 positionShadowIndex;
	vec4 color;
	vec4 ambientSpecularExponent;
	vec4 directionInnerCutoffAngle;
	vec4 attenuationOuterCutoffAngle;
};

struct DirectionalLightData
{
	vec4 positionShadowIndex;
	vec4 color;
	vec4 ambientSpecularExponent;
	vec4 direction;
};

layout(std140, set = 1, binding = 1) readonly buffer PointLightBuffer
{
	PointLightData lights[];
} pointLightBuffer;

layout(std140, set = 1, binding = 2) readonly buffer SpotLightBuffer
{
	SpotLightData lights[];
} spotLightBuffer;

layout(std140, set = 1, binding = 3) readonly buffer DirectionalLightBuffer
{
	DirectionalLightData lights[];
} dirLightBuffer;

struct ShadowData
{
	vec4 shadowBiasCascadeIndexAndCount;
};

layout(std140, set = 1, binding = 4) readonly buffer ShadowBuffer
{
	ShadowData shadows[];
} shadowBuffer;

struct ShadowCascadeData
{
	mat4 lightSpaceView;
	float cascadePlaneDistance;
};

layout(std140, set = 1, binding = 5) readonly buffer ShadowCascadeBuffer
{
	ShadowCascadeData cascades[];
} shadowCascadeBuffer;

const int maxTexturesPerMaterial = 8;
const int maxFloatsPerMaterial = 8;

struct MaterialData
{
	int texIndices[maxTexturesPerMaterial];
	float data[maxFloatsPerMaterial];
};

layout(set = 1, binding = 6) readonly buffer MaterialBuffer
{
	MaterialData materials[];
} materialBuffer;

layout(set = 2, binding = 0) uniform sampler2D textures[];
layout(set = 3, binding = 0) uniform sampler2D shadowmaps[];

layout( push_constant ) uniform constants
{	
	layout(offset = 0) vec4 viewPos;
	layout(offset = 16) vec4 lightCount;
} pushConstants;

vec3 PointLightCalculation(PointLightData lightData, vec3 fragNormal, vec3 viewDir, vec4 fragWorldPos);
vec3 SpotLightCalculation(SpotLightData lightData, vec3 fragNormal, vec3 viewDir, vec4 fragWorldPos);
vec3 DirLightCalculation(DirectionalLightData lightData, vec3 fragNormal, vec3 viewDir, vec4 fragWorldPos);
float ShadowCalculation(ShadowData shadowData, vec3 lightDir, vec3 fragNormal, vec4 fragWorldPos);

void main()
{
	MaterialData matData = materialBuffer.materials[matIndex];
	int albedoIdx = matData.texIndices[0];

	vec4 albedo = texture(textures[albedoIdx], fUV);

	vec3 viewDir = normalize(pushConstants.viewPos.rgb - fWorldPos.rgb);
	
	vec3 result = vec3(0.0);
	
	int pointLightCount = int(pushConstants.lightCount.x);
	for (int i = 0; i < pointLightCount; i++)
	{
		PointLightData lightData = pointLightBuffer.lights[i];
	
		result += PointLightCalculation(lightData, fNormal, viewDir, fWorldPos);
	}
	
	int spotLightCount = int(pushConstants.lightCount.y);
	for (int i = 0; i < spotLightCount; i++)
	{
		SpotLightData lightData = spotLightBuffer.lights[i];
	
		result += SpotLightCalculation(lightData, fNormal, viewDir, fWorldPos);
	}
	
	int dirLightCount = int(pushConstants.lightCount.z);
	for (int i = 0; i < dirLightCount; i++)
	{
		DirectionalLightData lightData = dirLightBuffer.lights[i];
	
		result += DirLightCalculation(lightData, fNormal, viewDir, fWorldPos);
	}

	outColor = vec4(albedo.rgb * result, 1.0);
}

vec3 PointLightCalculation(PointLightData lightData, vec3 fragNormal, vec3 viewDir, vec4 fragWorldPos)
{
	vec3 lightDir = normalize(lightData.positionShadowIndex.rgb - fragWorldPos.rgb);
	vec3 halfwayDir = normalize(lightDir + viewDir);

	// Diffuse Shading
	float diff = clamp(dot(fragNormal, lightDir), 0.0, 1.0);

	// Specular Shading
	vec3 reflectDir = reflect(-lightDir, fragNormal);
	float spec = pow(clamp(dot(fragNormal, halfwayDir), 0.0, 1.0), int(lightData.ambientSpecularExponent.z));

	// Attenuation
	float distance = length(lightData.positionShadowIndex.rgb - fragWorldPos.rgb);
	float attenuation = 1.0 / (lightData.attenuation.x + lightData.attenuation.y * distance + 
		lightData.attenuation.z * (distance * distance));

	vec3 diffuse = lightData.color.rgb * diff * attenuation;
	vec3 ambient = lightData.color.rgb * lightData.ambientSpecularExponent.x * attenuation;
	vec3 specular = lightData.color.rgb * lightData.ambientSpecularExponent.y * spec * attenuation;

	return diffuse + ambient + specular;
}

vec3 SpotLightCalculation(SpotLightData lightData, vec3 fragNormal, vec3 viewDir, vec4 fragWorldPos)
{
	vec3 lightDir = normalize(-lightData.directionInnerCutoffAngle.rgb);
	vec3 fragToLight = lightData.positionShadowIndex.rgb - fragWorldPos.rgb;

	vec3 fragToLightDir = normalize(fragToLight);
	vec3 halfwayDir = normalize(fragToLightDir + viewDir);

	// Diffuse Shading
	float diff = clamp(dot(fragNormal, fragToLightDir), 0.0, 1.0);

	// Specular Shading
	vec3 reflectDir = reflect(-lightDir, fragNormal);
	float spec = pow(clamp(dot(fragNormal, halfwayDir), 0.0, 1.0), int(lightData.ambientSpecularExponent.z));

	// Attenuation
	float distance = length(lightData.positionShadowIndex.rgb - fragWorldPos.rgb);
	float attenuation = 1.0 / (lightData.attenuationOuterCutoffAngle.x + lightData.attenuationOuterCutoffAngle.y * distance + 
		lightData.attenuationOuterCutoffAngle.z * (distance * distance));

	// Light Cutoff
	float theta = dot(fragToLightDir, normalize(-lightData.directionInnerCutoffAngle.rgb));
	float epsilon = lightData.directionInnerCutoffAngle.w - lightData.attenuationOuterCutoffAngle.w;
	float intensity = clamp((theta - lightData.attenuationOuterCutoffAngle.w) / epsilon, 0.0, 1.0);

	vec3 diffuse = lightData.color.rgb * diff * attenuation * intensity;
	vec3 ambient = lightData.color.rgb * lightData.ambientSpecularExponent.x * attenuation;
	vec3 specular = lightData.color.rgb * lightData.ambientSpecularExponent.y * spec * attenuation * intensity;

	float shadow = 0.0;
	
	int shadowIndex = int(lightData.positionShadowIndex.w);
	if (shadowIndex >= 0)
	{
		//shadow = ShadowCalculation(shadowBuffer.shadows[shadowIndex], lightDir, fragNormal, fragWorldPos);
	}

	return diffuse + ambient + specular;
	//return ((1.0 - shadow) * (diffuse + specular)) + ambient;
}

vec3 DirLightCalculation(DirectionalLightData lightData, vec3 fragNormal, vec3 viewDir, vec4 fragWorldPos)
{
	vec3 lightDir = normalize(-lightData.direction.rgb);
	vec3 halfwayDir = normalize(lightDir + viewDir);
	
	// Diffuse Shading
	float diff = clamp(dot(fragNormal, lightDir), 0.0, 1.0);
	
	// Specular Shading
	vec3 reflectDir = reflect(-lightDir, fragNormal);
	float spec = pow(clamp(dot(fragNormal, halfwayDir), 0.0, 1.0), int(lightData.ambientSpecularExponent.z));
	
	// Combine
	vec3 diffuse = lightData.color.rgb * diff;
	vec3 ambient = lightData.color.rgb * lightData.ambientSpecularExponent.x;
	vec3 specular = lightData.color.rgb * lightData.ambientSpecularExponent.y * spec;
	
	float shadow = 0.0;
	
	int shadowIndex = int(lightData.positionShadowIndex.w);
	if (shadowIndex >= 0)
	{
		//shadow = ShadowCalculation(shadowBuffer.shadows[shadowIndex], lightDir, fragNormal, fragWorldPos);
	}

	return diffuse + ambient + specular;
	//return ((1.0 - shadow) * (diffuse + specular)) + ambient;
}

float ShadowCalculation(ShadowData shadowData, vec3 lightDir, vec3 fragNormal, vec4 fragWorldPos)
{
	int shadowCascadeFirstIndex = int(shadowData.shadowBiasCascadeIndexAndCount.z);
	ShadowCascadeData shadowCascade = shadowCascadeBuffer.cascades[shadowCascadeFirstIndex];
	vec4 fragLightSpacePos = shadowCascade.lightSpaceView * fragWorldPos;
	vec3 projCoords = fragLightSpacePos.xyz / fragLightSpacePos.w;
	projCoords = projCoords * 0.5 + 0.5;
	
	float currentdepth = projCoords.z;
	
	float bias = max(shadowData.shadowBiasCascadeIndexAndCount.y * (1.0 - dot(fragNormal, lightDir)), shadowData.shadowBiasCascadeIndexAndCount.x);
	
	float shadow = 0.0;
	
	vec2 texelSize = 1.0 / textureSize(shadowmaps[shadowCascadeFirstIndex], 0);
	for (int x = -1; x <= 1; ++x)
	{
		for (int y = -1; y <= 1; ++y)
		{
			float pcfDepth = texture(shadowmaps[shadowCascadeFirstIndex], projCoords.xy + vec2(x, y) * texelSize).r;
			shadow += currentdepth - bias > pcfDepth ? 1.0 : 0.0;
		}
	}
	
	shadow /= 9.0;
	
	if (projCoords.z > 1.0)
		shadow = 0.0;

	return shadow;
}