$input a_position, a_normal, a_tangent, a_texcoord0, i_data0, i_data1, i_data2, i_data3, i_data4
$output v_normal, v_tangent, v_bitangent, v_texcoord0, v_wpos, v_view, v_texIndex

/*
 * Copyright 2011-2022 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx/blob/master/LICENSE
 */

#include "../common/common.sh"

void main()
{
	mat4 model = mtxFromCols(i_data0, i_data1, i_data2, i_data3);

	// Calculate World Position
	vec4 wpos = mul(model, vec4(a_position, 1.0));
	v_wpos = wpos.xyz;
	
	gl_Position = mul(u_viewProj, wpos );
	
	// Calculate Normal, Tangent & Bitangent
	vec4 normal = a_normal * 2.0 - 1.0;
	vec4 tangent = a_tangent * 2.0 - 1.0;
	
	vec3 wnormal = mul(u_model[0], vec4(normal.xyz, 0.0) ).xyz;
	vec3 wtangent = mul(u_model[0], vec4(tangent.xyz, 0.0) ).xyz;

	v_normal = normalize(wnormal);
	v_tangent = normalize(wtangent);
	v_bitangent = cross(v_normal, v_tangent) * tangent.w;

	mat3 tbn = mtxFromCols(v_tangent, v_bitangent, v_normal);
	
	// Eye position in world space
	vec3 weyepos = mul(vec4(0.0, 0.0, 0.0, 1.0), u_view).xyz;
	
	// Calculate tangent space view dir
	v_view = mul(weyepos - wpos.xyz, tbn);
	
	// Texture Coordinate
	v_texcoord0 = a_texcoord0;
	
	v_texIndex = i_data4;
}
