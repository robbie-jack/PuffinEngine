#version 460
#extension GL_EXT_nonuniform_qualifier : enable

layout(set = 0, binding = 2) uniform sampler2D samplerColor[];
layout(set = 0, binding = 3) uniform sampler2D samplerNormalMap[];

layout(location = 0) in vec4 fragWorldPos;
layout(location = 1) in vec3 fragColor;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragTangent;
layout(location = 4) in vec2 fragUV;
layout(location = 5) flat in int fragInstance;

layout(location = 0) out vec4 outPosition;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outAlbedoSpec;

void main()
{
	outPosition = vec4(fragWorldPos.xyz, 1.0);

	// Calculate Normal in Tangent Space
	vec3 N = normalize(fragNormal);
	vec3 T = normalize(fragTangent);
	vec3 B = cross(N, T);
	mat3 TBN = mat3(T, B, N);
	vec3 tnorm = TBN * normalize(texture(samplerNormalMap[fragInstance], fragUV).xyz * 2.0 - vec3(1.0));
	outNormal = vec4(tnorm, 1.0);

	outAlbedoSpec = texture(samplerColor[fragInstance], fragUV);
}