$input v_normal, v_tangent, v_bitangent, v_texcoord0, v_wpos, v_view, v_texIndex

/*
 * Copyright 2011-2022 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx/blob/master/LICENSE
 */

#include "../common/common.sh"

#define MAX_LIGHTS 12

SAMPLER2DARRAY(s_texColor,  0);
//SAMPLER2DARRAY(s_texNormal, 1);

uniform vec4 u_lightPos[MAX_LIGHTS];
uniform vec4 u_lightDir[MAX_LIGHTS];
uniform vec4 u_lightColor[MAX_LIGHTS];
uniform vec4 u_lightAmbientSpecular[MAX_LIGHTS];
uniform vec4 u_lightAttenuation[MAX_LIGHTS];
uniform vec4 u_lightIndex;

vec3 CalcDirLight(int lightIndex, vec3 normal, vec3 viewDir);
vec3 CalcPointLight(int lightIndex, vec3 normal, vec3 viewDir, vec3 fragPos);
vec3 CalcSpotLight(int lightIndex, vec3 normal, vec3 viewDir, vec3 fragPos);

void main()
{
	vec3 texCoord = vec3(v_texcoord0.xy, v_texIndex);
	vec4 albedo = toLinear(texture2DArray(s_texColor, texCoord));
	
	vec3 result = vec3(0.0, 0.0, 0.0);
	
	int dirLightIndex = int(u_lightIndex.x);
	int	pointLightIndex = int(u_lightIndex.y);
	int spotLightIndex = int(u_lightIndex.z);
	
	vec3 viewDir = normalize(v_view - v_wpos);
	
	// Calculate Dir Lights
	for (int i = 0; i < dirLightIndex; i++)
	{
		result += CalcDirLight(i, v_normal, viewDir);
	}
	
	// Calculate Point Lights
	for (int i = dirLightIndex; i < pointLightIndex; i++)
	{
		result += CalcPointLight(i, v_normal, viewDir, v_wpos);
	}
	
	// Calculate Spot Lights
	for (int i = pointLightIndex; i < spotLightIndex; i++)
	{
		result += CalcSpotLight(i, v_normal, viewDir, v_wpos);
	}

	gl_FragColor = vec4(albedo.rgb * result, 1.0);
}

vec3 CalcDirLight(int index, vec3 normal, vec3 viewDir)
{
	vec3 lightDir = normalize(-u_lightDir[index]);
	
	// Diffuse Shading
	float diff = max(dot(normal, lightDir), 0.0);
	
	// Specular Shading
	vec3 reflectDir = reflect(-lightDir, normal);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), (int)u_lightAmbientSpecular[index].z);
	
	// Combine
	vec3 ambient = u_lightAmbientSpecular[index].x * u_lightColor[index];
	vec3 diffuse = u_lightColor[index] * diff;
	vec3 specular = u_lightAmbientSpecular[index].y * spec;

	return vec3(ambient + diffuse + specular);
}

vec3 CalcPointLight(int index, vec3 normal, vec3 viewDir, vec3 fragPos)
{
	return vec3(0.0);
}

vec3 CalcSpotLight(int index, vec3 normal, vec3 viewDir, vec3 fragPos)
{
	return vec3(0.0);
}