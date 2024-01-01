#version 460
#extension GL_EXT_buffer_reference : require

layout (location = 0) out vec4 fWorldPos;
layout (location = 1) out vec3 fNormal;
layout (location = 2) out vec3 fTangent;
layout (location = 3) out vec2 fUV;
layout (location = 4) flat out int matIndex;

layout(set = 0, binding = 0) uniform CameraBuffer
{
	mat4 view;
	mat4 proj;
	mat4 viewProj;
} cameraData;

struct ObjectData
{
	mat4 model;
	int matIndex;
};

layout(std140, set = 0, binding = 1) readonly buffer ObjectBuffer
{
	ObjectData objects[];
} objectBuffer;

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
	VertexBuffer vertexBuffer;
} PushConstants;

void main()
{
	Vertex v = PushConstants.vertexBuffer.vertices[gl_VertexIndex];
	
	mat4 modelMatrix = objectBuffer.objects[gl_BaseInstance].model;
	mat4 modelMatrixInv = inverse(modelMatrix);
	mat4 viewProjMatrix = cameraData.viewProj;
		
	fWorldPos = modelMatrix * vec4(v.position, 1.0f);
	fUV = vec2(v.uvX, v.uvY);

	mat3 mNormal = transpose(mat3(modelMatrixInv));
	fNormal = normalize(mNormal * v.normal);
	fTangent = normalize(mNormal * v.tangent);
	//fNormal = (viewProjMatrix * vec4(v.normal, 1.0)).rgb;
	//fTangent = (viewProjMatrix * vec4(v.tangent, 1.0)).rgb;
	
	matIndex = objectBuffer.objects[gl_BaseInstance].matIndex;

	gl_Position = viewProjMatrix * fWorldPos;
}