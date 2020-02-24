#include "Mesh.h"

Mesh::Mesh()
{

}

Mesh::~Mesh()
{

}

void Mesh::SetupMesh(std::vector<Vertex> vertices_, std::vector<uint32_t> indices_)
{
	vertices = vertices_;
	indices = indices_;
}

void Mesh::Cleanup(VkDevice device)
{
	vkDestroyBuffer(device, vertexBuffer, nullptr);
	vkFreeMemory(device, vertexBufferMemory, nullptr);

	vkDestroyBuffer(device, indexBuffer, nullptr);
	vkFreeMemory(device, indexBufferMemory, nullptr);
}