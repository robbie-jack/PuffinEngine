#version 460
//#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : enable

layout (location = 0) in vec4 fWorldPos;
layout (location = 1) in vec3 fNormal;
layout (location = 2) in vec3 fTangent;
layout (location = 3) in vec2 fUV;
layout (location = 4) flat in int matIndex;

layout (location = 0) out vec4 outColor;

struct LightData
{
	vec4 positionAndType;
	vec4 direction;
	vec4 color;
	vec4 ambientSpecular;
	vec4 attenuation;
	vec4 cutoffAngle;
};

layout(std140, set = 0, binding = 2) readonly buffer LightBuffer
{
	LightData lights[];
} lightBuffer;

layout(std140, set = 0, binding = 3) uniform LightStaticBuffer
{
	vec4 viewPosAndNumLights;
} lightStatic;

const int maxTexturesPerMaterial = 8;
const int maxFloatsPerMaterial = 8;

struct MaterialData
{
	int texIndices[maxTexturesPerMaterial];
	float data[maxFloatsPerMaterial];
};

layout(set = 0, binding = 4) readonly buffer MaterialBuffer
{
	MaterialData materials[];
} materialBuffer;

layout(set = 0, binding = 5) uniform sampler2D textures[];

vec3 CalcDirLight(LightData lightData, vec3 normal, vec3 viewDir);
vec3 CalcPointLight(LightData lightData, vec3 normal, vec3 viewDir, vec3 fragWorldPos);
vec3 CalcSpotLight(LightData lightData, vec3 normal, vec3 viewDir, vec3 fragWorldPos);

void main()
{
	MaterialData matData = materialBuffer.materials[matIndex];
	int albedoIdx = matData.texIndices[0];

	vec4 albedo = texture(textures[albedoIdx], fUV);

	vec3 viewDir = normalize(lightStatic.viewPosAndNumLights.rgb - fWorldPos.rgb);
	
	vec3 result = vec3(0.0);
	
	int numLights = int(lightStatic.viewPosAndNumLights.w);
	
	for (int i = 0; i < numLights; i++)
	{
		LightData lightData = lightBuffer.lights[i];

		int lightType = int(lightData.positionAndType.w);
		switch (lightType)
		{
			case 0:
				result += CalcPointLight(lightData, fNormal, viewDir, fWorldPos.rgb);
				break;
			case 1:
				result += CalcSpotLight(lightData, fNormal, viewDir, fWorldPos.rgb);
				break;
			case 2:
				result += CalcDirLight(lightData, fNormal, viewDir);
				break;
		}
	}

	outColor = vec4(albedo.rgb * result, 1.0);
}

vec3 CalcDirLight(LightData lightData, vec3 normal, vec3 viewDir)
{
	vec3 lightDir = normalize(-lightData.direction.rgb);
	vec3 halfwayDir = normalize(lightDir + viewDir);
	
	// Diffuse Shading
	float diff = clamp(dot(normal, lightDir), 0.0, 1.0);
	
	// Specular Shading
	vec3 reflectDir = reflect(-lightDir, normal);
	float spec = pow(clamp(dot(normal, halfwayDir), 0.0, 1.0), int(lightData.ambientSpecular.z));
	
	// Combine
	vec3 diffuse = lightData.color.rgb * diff;
	vec3 ambient = lightData.color.rgb * lightData.ambientSpecular.x;
	vec3 specular = lightData.color.rgb * lightData.ambientSpecular.y * spec;

	return diffuse + ambient + specular;
}

vec3 CalcPointLight(LightData lightData, vec3 normal, vec3 viewDir, vec3 fragWorldPos)
{
	vec3 lightDir = normalize(lightData.positionAndType.rgb - fragWorldPos);
	vec3 halfwayDir = normalize(lightDir + viewDir);

	// Diffuse Shading
	float diff = clamp(dot(normal, lightDir), 0.0, 1.0);

	// Specular Shading
	vec3 reflectDir = reflect(-lightDir, normal);
	float spec = pow(clamp(dot(normal, halfwayDir), 0.0, 1.0), int(lightData.ambientSpecular.z));

	// Attenuation
	float distance = length(lightData.positionAndType.rgb - fragWorldPos);
	float attenuation = 1.0 / (lightData.attenuation.x + lightData.attenuation.y * distance + 
		lightData.attenuation.z * (distance * distance));

	vec3 diffuse = lightData.color.rgb * diff * attenuation;
	vec3 ambient = lightData.color.rgb * lightData.ambientSpecular.x * attenuation;
	vec3 specular = lightData.color.rgb * lightData.ambientSpecular.y * spec * attenuation;

	return diffuse + ambient + specular;
}

vec3 CalcSpotLight(LightData lightData, vec3 normal, vec3 viewDir, vec3 fragWorldPos)
{
	vec3 lightDir = normalize(-lightData.direction.rgb);
	vec3 fragToLight = lightData.positionAndType.rgb - fragWorldPos;

	vec3 fragToLightDir = normalize(fragToLight);
	vec3 halfwayDir = normalize(fragToLightDir + viewDir);

	// Diffuse Shading
	float diff = clamp(dot(normal, fragToLightDir), 0.0, 1.0);

	// Specular Shading
	vec3 reflectDir = reflect(-lightDir, normal);
	float spec = pow(clamp(dot(normal, halfwayDir), 0.0, 1.0), int(lightData.ambientSpecular.z));

	// Attenuation
	float distance = length(lightData.positionAndType.rgb - fragWorldPos);
	float attenuation = 1.0 / (lightData.attenuation.x + lightData.attenuation.y * distance + 
		lightData.attenuation.z * (distance * distance));

	// Light Cutoff
	float theta = dot(fragToLightDir, normalize(-lightData.direction.rgb));
	float epsilon = lightData.cutoffAngle.x - lightData.cutoffAngle.y;
	float intensity = clamp((theta - lightData.cutoffAngle.y) / epsilon, 0.0, 1.0);

	vec3 diffuse = lightData.color.rgb * diff * attenuation * intensity;
	vec3 ambient = lightData.color.rgb * lightData.ambientSpecular.x * attenuation;
	vec3 specular = lightData.color.rgb * lightData.ambientSpecular.y * spec * attenuation * intensity;

	return diffuse + ambient + specular;
}