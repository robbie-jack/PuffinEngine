$input v_normal, v_tangent, v_bitangent, v_texcoord0, v_wpos, v_view

/*
 * Copyright 2011-2022 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx/blob/master/LICENSE
 */

#include "../common/common.sh"

#define MAX_LIGHTS 12

SAMPLER2D(s_texColor,  0);
//SAMPLER2D(s_texNormal, 1);



void main()
{
	vec4 color = toLinear(texture2D(s_texColor, v_texcoord0) );

	gl_FragColor = color;
}
