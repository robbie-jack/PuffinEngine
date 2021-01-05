#version 460

void main()
{
	gl_FragDepth = gl_FragCoord.z;
	//gl_FragDepth = 0.0;
}