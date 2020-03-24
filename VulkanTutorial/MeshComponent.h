#pragma once

#include "BaseComponent.h"
#include "Mesh.h"
#include "Texture.h"

#include <vector>

class MeshComponent : public BaseComponent
{
public:

	~MeshComponent();

private:

	Texture texture;

	MeshMatrices matrices;
	Transform transform;

	std::vector<Vertex> vertices;
	std::vector<glm::vec3> normals;
	std::vector<uint32_t> indices;

	// Vertex Buffer
	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;

	// Index Buffer
	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;

	// Uniform Buffers
	std::vector<VkBuffer> uniformBuffers;
	std::vector<VkDeviceMemory> uniformBuffersMemory;

	std::vector<VkDescriptorSet> descriptorSets;

	
};