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
	vec4 position_and_type;
	vec4 direction;
	vec4 color;
	vec4 ambient_specular;
	vec4 attenuation;
	vec4 cuttoff_angle_and_shadow_index;
};

layout(std140, set = 1, binding = 1) readonly buffer LightBuffer
{
	LightData lights[];
} lightBuffer;

struct ShadowData
{
	vec4 shadow_bias;
	int cascade_count;
};

layout(std140, set = 1, binding = 2) readonly buffer ShadowBuffer
{
	ShadowData shadows[];
} shadow_buffer;

struct ShadowCascadeData
{
	mat4 light_space_view;
	float cascade_plane_distance;
};

layout(std140, set = 1, binding = 3) readonly buffer ShadowCascadeBuffer
{
	ShadowCascadeData cascades[];
} shadow_cascade_buffer;

const int maxTexturesPerMaterial = 8;
const int maxFloatsPerMaterial = 8;

struct MaterialData
{
	int texIndices[maxTexturesPerMaterial];
	float data[maxFloatsPerMaterial];
};

layout(set = 1, binding = 4) readonly buffer MaterialBuffer
{
	MaterialData materials[];
} materialBuffer;

layout(set = 2, binding = 0) uniform sampler2D textures[];
layout(set = 3, binding = 0) uniform sampler2D shadowmaps[];

layout( push_constant ) uniform constants
{	
	layout(offset = 16) vec4 view_pos_and_light_count;
} push_constants;

float shadow_calculation(LightData light_data, ShadowData shadow_data, vec3 lightDir, vec3 fragNormal, vec4 fragWorldPos);
vec3 dir_light_calculation(LightData lightData, vec3 fragNormal, vec3 viewDir, vec4 fragWorldPos);
vec3 point_light_calculation(LightData lightData, vec3 fragNormal, vec3 viewDir, vec4 fragWorldPos);
vec3 spot_light_calculation(LightData lightData, vec3 fragNormal, vec3 viewDir, vec4 fragWorldPos);

void main()
{
	MaterialData matData = materialBuffer.materials[matIndex];
	int albedoIdx = matData.texIndices[0];

	vec4 albedo = texture(textures[albedoIdx], fUV);

	vec3 viewDir = normalize(push_constants.view_pos_and_light_count.rgb - fWorldPos.rgb);
	
	vec3 result = vec3(0.0);
	
	int light_count = int(push_constants.view_pos_and_light_count.w);
	
	for (int i = 0; i < light_count; i++)
	{
		LightData lightData = lightBuffer.lights[i];

		int lightType = int(lightData.position_and_type.w);
		switch (lightType)
		{
			case 0:
				result += point_light_calculation(lightData, fNormal, viewDir, fWorldPos);
				break;
			case 1:
				result += spot_light_calculation(lightData, fNormal, viewDir, fWorldPos);
				break;
			case 2:
				result += dir_light_calculation(lightData, fNormal, viewDir, fWorldPos);
				break;
		}
	}

	outColor = vec4(albedo.rgb * result, 1.0);
}

float shadow_calculation(LightData light_data, ShadowData shadow_data, vec3 lightDir, vec3 fragNormal, vec4 fragWorldPos)
{
	int shadow_index = int(light_data.cuttoff_angle_and_shadow_index.z);
	vec4 fragLightSpacePos = shadow_data.light_space_view * fragWorldPos;
	vec3 projCoords = fragLightSpacePos.xyz / fragLightSpacePos.w;
	projCoords = projCoords * 0.5 + 0.5;
	
	float currentdepth = projCoords.z;
	
	float bias = max(shadow_data.shadow_bias.y * (1.0 - dot(fragNormal, lightDir)), shadow_data.shadow_bias.x);
	
	float shadow = 0.0;
	
	vec2 texelSize = 1.0 / textureSize(shadowmaps[int(shadow_index)], 0);
	for (int x = -1; x <= 1; ++x)
	{
		for (int y = -1; y <= 1; ++y)
		{
			float pcfDepth = texture(shadowmaps[int(shadow_index)], projCoords.xy + vec2(x, y) * texelSize).r;
			shadow += currentdepth - bias > pcfDepth ? 1.0 : 0.0;
		}
	}
	
	shadow /= 9.0;
	
	if (projCoords.z > 1.0)
		shadow = 0.0;

	return shadow;
}

vec3 dir_light_calculation(LightData lightData, vec3 fragNormal, vec3 viewDir, vec4 fragWorldPos)
{
	vec3 lightDir = normalize(-lightData.direction.rgb);
	vec3 halfwayDir = normalize(lightDir + viewDir);
	
	// Diffuse Shading
	float diff = clamp(dot(fragNormal, lightDir), 0.0, 1.0);
	
	// Specular Shading
	vec3 reflectDir = reflect(-lightDir, fragNormal);
	float spec = pow(clamp(dot(fragNormal, halfwayDir), 0.0, 1.0), int(lightData.ambient_specular.z));
	
	// Combine
	vec3 diffuse = lightData.color.rgb * diff;
	vec3 ambient = lightData.color.rgb * lightData.ambient_specular.x;
	vec3 specular = lightData.color.rgb * lightData.ambient_specular.y * spec;
	
	float shadow = 0.0;
	
	int shadow_index = int(lightData.cuttoff_angle_and_shadow_index.z);
	if (shadow_index >= 0)
	{
		shadow = shadow_calculation(lightData, shadow_buffer.shadows[shadow_index], lightDir, fragNormal, fragWorldPos);
	}

	//return diffuse + ambient + specular;
	return ((1.0 - shadow) * (diffuse + specular)) + ambient;
}

vec3 point_light_calculation(LightData lightData, vec3 fragNormal, vec3 viewDir, vec4 fragWorldPos)
{
	vec3 lightDir = normalize(lightData.position_and_type.rgb - fragWorldPos.rgb);
	vec3 halfwayDir = normalize(lightDir + viewDir);

	// Diffuse Shading
	float diff = clamp(dot(fragNormal, lightDir), 0.0, 1.0);

	// Specular Shading
	vec3 reflectDir = reflect(-lightDir, fragNormal);
	float spec = pow(clamp(dot(fragNormal, halfwayDir), 0.0, 1.0), int(lightData.ambient_specular.z));

	// Attenuation
	float distance = length(lightData.position_and_type.rgb - fragWorldPos.rgb);
	float attenuation = 1.0 / (lightData.attenuation.x + lightData.attenuation.y * distance + 
		lightData.attenuation.z * (distance * distance));

	vec3 diffuse = lightData.color.rgb * diff * attenuation;
	vec3 ambient = lightData.color.rgb * lightData.ambient_specular.x * attenuation;
	vec3 specular = lightData.color.rgb * lightData.ambient_specular.y * spec * attenuation;

	return diffuse + ambient + specular;
}

vec3 spot_light_calculation(LightData lightData, vec3 fragNormal, vec3 viewDir, vec4 fragWorldPos)
{
	vec3 lightDir = normalize(-lightData.direction.rgb);
	vec3 fragToLight = lightData.position_and_type.rgb - fragWorldPos.rgb;

	vec3 fragToLightDir = normalize(fragToLight);
	vec3 halfwayDir = normalize(fragToLightDir + viewDir);

	// Diffuse Shading
	float diff = clamp(dot(fragNormal, fragToLightDir), 0.0, 1.0);

	// Specular Shading
	vec3 reflectDir = reflect(-lightDir, fragNormal);
	float spec = pow(clamp(dot(fragNormal, halfwayDir), 0.0, 1.0), int(lightData.ambient_specular.z));

	// Attenuation
	float distance = length(lightData.position_and_type.rgb - fragWorldPos.rgb);
	float attenuation = 1.0 / (lightData.attenuation.x + lightData.attenuation.y * distance + 
		lightData.attenuation.z * (distance * distance));

	// Light Cutoff
	float theta = dot(fragToLightDir, normalize(-lightData.direction.rgb));
	float epsilon = lightData.cuttoff_angle_and_shadow_index.x - lightData.cuttoff_angle_and_shadow_index.y;
	float intensity = clamp((theta - lightData.cuttoff_angle_and_shadow_index.y) / epsilon, 0.0, 1.0);

	vec3 diffuse = lightData.color.rgb * diff * attenuation * intensity;
	vec3 ambient = lightData.color.rgb * lightData.ambient_specular.x * attenuation;
	vec3 specular = lightData.color.rgb * lightData.ambient_specular.y * spec * attenuation * intensity;

	float shadow = 0.0;
	
	int shadow_index = int(lightData.cuttoff_angle_and_shadow_index.z);
	if (shadow_index >= 0)
	{
		shadow = shadow_calculation(lightData, shadow_buffer.shadows[shadow_index], lightDir, fragNormal, fragWorldPos);
	}

	//return diffuse + ambient + specular;
	return ((1.0 - shadow) * (diffuse + specular)) + ambient;
}