#version 460
//#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : enable

layout (location = 0) in vec3 fPos;
layout (location = 1) in vec3 fNormal;
layout (location = 2) in vec3 fTangent;
layout (location = 3) in vec2 fUV;
layout (location = 4) flat in int texIndex;

layout (location = 0) out vec4 outColor;

layout(set = 0, binding = 2) uniform sampler2D textures[];

void main()
{
	//outColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
	outColor = texture(textures[texIndex], fUV);
}