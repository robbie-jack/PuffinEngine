#version 460
#extension GL_EXT_buffer_reference : require

struct ObjectData
{
	mat4 model;
	int mat_index;
};

layout(std140, set = 0, binding = 1) readonly buffer ObjectBuffer
{
	ObjectData objects[];
} object_buffer;

struct Vertex
{
	vec3 position;
	float uvX;
	vec3 normal;
	float uvY;
	vec3 tangent;
};

layout(buffer_reference, std430) readonly buffer VertexBuffer{ 
	Vertex vertices[];
};

layout( push_constant ) uniform constants
{	
	VertexBuffer vertex_buffer;
	mat4 light_space_view;
} push_contants;

void main()
{
	Vertex v = push_contants.vertex_buffer.vertices[gl_VertexIndex];

	gl_Position = push_contants.light_space_view * object_buffer.objects[gl_InstanceIndex].model * vec4(v.position, 1.0);
}