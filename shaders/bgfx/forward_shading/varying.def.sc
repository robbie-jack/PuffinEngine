vec3 a_position  : POSITION;
vec4 a_normal    : NORMAL;
vec4 a_tangent   : TANGENT;
vec2 a_texcoord0 : TEXCOORD0;
vec4 i_data0	 : TEXCOORD1;
vec4 i_data1	 : TEXCOORD2;
vec4 i_data2	 : TEXCOORD3;
vec4 i_data3	 : TEXCOORD4;
vec4 i_data4     : TEXCOORD5;

vec3 v_normal    : NORMAL    = vec3(0.0, 0.0, 1.0);
vec3 v_tangent	 : TANGENT	 = vec3(1.0, 0.0, 0.0);
vec3 v_bitangent : BINORMAL	 = vec3(0.0, 1.0, 0.0);
vec2 v_texcoord0 : TEXCOORD0 = vec2(0.0, 0.0);
vec3 v_wpos      : TEXCOORD1 = vec3(0.0, 0.0, 0.0);
vec3 v_view      : TEXCOORD2 = vec3(0.0, 0.0, 0.0);
vec4 v_instIndex : TEXCOORD3 = vec4(0.0, 0.0, 0.0, 0.0);