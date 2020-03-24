#include "Mesh.h"

Mesh::Mesh()
{
	transform.position = { 0.0f, 0.0f, 0.0f };
	transform.rotation = { 0.0f, 0.0f, 0.0f };
	transform.scale = { 1.0f, 1.0f, 1.0f };
}

Mesh::~Mesh()
{

}

void Mesh::SetupMesh(std::vector<Vertex> vertices_, std::vector<uint32_t> indices_)
{
	meshData.vertices = vertices_;
	meshData.indices = indices_;
}

void Mesh::Cleanup(VkDevice device)
{
	texture.Cleanup(device);

	vkDestroyBuffer(device, vertexBuffer, nullptr);
	vkFreeMemory(device, vertexBufferMemory, nullptr);

	vkDestroyBuffer(device, indexBuffer, nullptr);
	vkFreeMemory(device, indexBufferMemory, nullptr);

	for (int i = 0; i < uniformBuffers.size(); i++)
	{
		vkDestroyBuffer(device, uniformBuffers[i], nullptr);
		vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
	}
	
}

void Mesh::SetColor(glm::vec3 color)
{
	for (int i = 0; i < meshData.vertices.size(); i++)
	{
		meshData.vertices[i].color = color;
	}
}