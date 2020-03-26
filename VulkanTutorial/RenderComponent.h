#pragma once

#include "BaseComponent.h"
#include "Mesh.h"
#include "Texture.h"

#include <vector>

class RenderComponent : public BaseComponent
{
public:

	~RenderComponent();

	inline Texture& GetTexture() { return texture; };
	inline void SetTexture(Texture texture_) { texture = texture_; };

	inline Mesh& GetMesh() { return mesh; };

	inline VkBuffer& GetVertexBuffer() { return vertexBuffer; };
	inline VkBuffer& GetIndexBuffer() { return indexBuffer; };
	inline VkBuffer& GetUniformBuffer(int i) { return uniformBuffers[i]; };

	inline VkDeviceMemory& GetVertexMemory() { return vertexBufferMemory; };
	inline VkDeviceMemory& GetIndexMemory() { return indexBufferMemory; };
	inline VkDeviceMemory& GetUniformBufferMemory(int i) { return uniformBuffersMemory[i]; };

	inline std::vector<VkDescriptorSet>& GetDescriptorSets() { return descriptorSets; };
	inline std::vector<VkBuffer>& GetUniformBufferVector() { return uniformBuffers; };
	inline std::vector<VkDeviceMemory>& GetUniformMemoryVector() { return uniformBuffersMemory; };

private:
	Mesh mesh;
	Texture texture;

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