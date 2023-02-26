#version 460
//#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : enable

layout (location = 0) in vec4 fWorldPos;
layout (location = 1) in vec3 fNormal;
layout (location = 2) in vec3 fTangent;
layout (location = 3) in vec2 fUV;
layout (location = 4) flat in int texIndex;

layout (location = 0) out vec4 outColor;

struct LightData
{
	vec3 position;
	vec3 direction;
	vec3 color;
	vec3 ambientSpecular;
	vec3 attenuation;
	vec3 cutoffAngle;
	int type;
};

layout(std140, set = 0, binding = 2) readonly buffer LightBuffer
{
	LightData lights[];
} lightBuffer;

layout(set = 0, binding = 3) uniform LightStaticBuffer
{
	vec3 viewPos;
	int numLights;
} lightStatic;

layout(set = 0, binding = 4) uniform sampler2D textures[];

vec3 CalcDirLight(LightData lightData, vec3 normal, vec3 viewDir);
vec3 CalcPointLight(LightData lightData, vec3 normal, vec3 viewDir, vec3 fragWorldPos);
vec3 CalcSpotLight(LightData lightData, vec3 normal, vec3 viewDir, vec3 fragWorldPos);

void main()
{
	vec4 albedo = texture(textures[texIndex], fUV);

	vec3 viewDir = normalize(lightStatic.viewPos - fWorldPos.rgb);
	
	vec3 result = vec3(0.0);

	for (int i = 0; i < lightStatic.numLights; i++)
	{
		LightData lightData = lightBuffer.lights[i];

		switch (lightData.type)
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
	vec3 lightDir = normalize(-lightData.direction);
	vec3 halfwayDir = normalize(lightDir + viewDir);
	
	// Diffuse Shading
	float diff = clamp(dot(normal, lightDir), 0.0, 1.0);
	
	// Specular Shading
	vec3 reflectDir = reflect(-lightDir, normal);
	float spec = pow(clamp(dot(normal, halfwayDir), 0.0, 1.0), int(lightData.ambientSpecular.z));
	
	// Combine
	vec3 diffuse = lightData.color * diff;
	vec3 ambient = lightData.color * lightData.ambientSpecular.x;
	vec3 specular = lightData.color * lightData.ambientSpecular.y * spec;

	return diffuse + ambient + specular;
}

vec3 CalcPointLight(LightData lightData, vec3 normal, vec3 viewDir, vec3 fragWorldPos)
{
	vec3 lightDir = normalize(lightData.position - fragWorldPos);
	vec3 halfwayDir = normalize(lightDir + viewDir);

	// Diffuse Shading
	float diff = clamp(dot(normal, lightDir), 0.0, 1.0);

	// Specular Shading
	vec3 reflectDir = reflect(-lightDir, normal);
	float spec = pow(clamp(dot(normal, halfwayDir), 0.0, 1.0), int(lightData.ambientSpecular.z));

	// Attenuation
	float distance = length(lightData.position - fragWorldPos);
	float attenuation = 1.0 / (lightData.attenuation.x + lightData.attenuation.y * distance + 
		lightData.attenuation.z * (distance * distance));

	vec3 diffuse = lightData.color * diff * attenuation;
	vec3 ambient = lightData.color * lightData.ambientSpecular.x * attenuation;
	vec3 specular = lightData.color * lightData.ambientSpecular.y * spec * attenuation;

	return diffuse + ambient + specular;
}

vec3 CalcSpotLight(LightData lightData, vec3 normal, vec3 viewDir, vec3 fragWorldPos)
{
	return vec3(0.0);
}