#version 460
#extension GL_EXT_buffer_reference : require

layout (location = 0) out vec4 fWorldPos;
layout (location = 1) out vec3 fNormal;
layout (location = 2) out vec3 fTangent;
layout (location = 3) out vec2 fUV;
layout (location = 4) flat out int mat_index;

struct ObjectData
{
	mat4 model;
	int mat_index;
};

layout(std140, set = 0, binding = 0) readonly buffer ObjectBuffer
{
	ObjectData objects[];
} object_buffer;

layout(set = 1, binding = 0) uniform CameraBuffer
{
	mat4 view;
	mat4 proj;
	mat4 view_proj;
} camera_data;

struct Vertex
{
	vec3 position;
	float uv_x;
	vec3 normal;
	float uv_y;
	vec3 tangent;
};

layout(buffer_reference, std430) readonly buffer VertexBuffer{ 
	Vertex vertices[];
};

layout( push_constant ) uniform constants
{	
	VertexBuffer vertexBuffer;
} push_constants;

void main()
{
	Vertex v = push_constants.vertexBuffer.vertices[gl_VertexIndex];
	
	mat4 modelMatrix = object_buffer.objects[gl_InstanceIndex].model;
	mat4 modelMatrixInv = inverse(modelMatrix);
	mat4 view_projMatrix = camera_data.view_proj;
		
	fWorldPos = modelMatrix * vec4(v.position, 1.0f);
	fUV = vec2(v.uv_x, v.uv_y);

	mat3 mNormal = transpose(mat3(modelMatrixInv));
	fNormal = normalize(mNormal * v.normal);
	fTangent = normalize(mNormal * v.tangent);
	//fNormal = (view_projMatrix * vec4(v.normal, 1.0)).rgb;
	//fTangent = (view_projMatrix * vec4(v.tangent, 1.0)).rgb;
	
	mat_index = object_buffer.objects[gl_InstanceIndex].mat_index;

	gl_Position = view_projMatrix * fWorldPos;
}