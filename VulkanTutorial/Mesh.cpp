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
	vertices = vertices_;
	indices = indices_;
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

void Mesh::BuildTransform()
{
	// Translation
	matrices.model = glm::translate(glm::mat4(1.0f), transform.position);

	// Rotation
	matrices.model = glm::rotate(matrices.model, glm::radians(transform.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
	matrices.model = glm::rotate(matrices.model, glm::radians(transform.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
	matrices.model = glm::rotate(matrices.model, glm::radians(transform.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

	// Scale
	matrices.model = glm::scale(matrices.model, transform.scale);
}