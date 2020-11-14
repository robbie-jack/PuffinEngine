#pragma once

#include "Mesh.h"
#include "Texture.h"

#include <vector>

namespace Puffin
{
	namespace Rendering
	{
		//class MeshComponent
		//{
		//public:
		//	inline Texture& GetTexture() { return texture; };
		//	inline void SetTexture(Texture texture_) { texture = texture_; };

		//	inline Mesh& GetMesh() { return mesh; };

		//	inline VkBuffer& GetVertexBuffer() { return vertexBuffer; };
		//	inline VkBuffer& GetIndexBuffer() { return indexBuffer; };
		//	inline VkBuffer& GetUniformBuffer(int i) { return uniformBuffers[i]; };

		//	inline VmaAllocation& GetVertexAllocation() { return vertexAllocation; };
		//	inline VmaAllocation& GetIndexAllocation() { return indexAllocation; };
		//	inline VmaAllocation& GetUniformAllocation(int i) { return uniformAllocations[i]; };

		//	inline std::vector<VkDescriptorSet>& GetDescriptorSets() { return descriptorSets; };
		//	inline std::vector<VkBuffer>& GetUniformBufferVector() { return uniformBuffers; };
		//	inline std::vector<VmaAllocation>& GetUniformAllocationsVector() { return uniformAllocations; };

		//protected:
		//	Mesh mesh;
		//	Texture texture;

		//	// Vertex Buffer
		//	VkBuffer vertexBuffer;
		//	VmaAllocation vertexAllocation;

		//	// Index Buffer
		//	VkBuffer indexBuffer;
		//	VmaAllocation indexAllocation;

		//	// Uniform Buffers
		//	std::vector<VkBuffer> uniformBuffers;
		//	std::vector<VmaAllocation> uniformAllocations;

		//	std::vector<VkDescriptorSet> descriptorSets;
		//};

		struct MeshComponent
		{
			Mesh mesh;
			Texture texture;

			// Vertex Buffer
			VkBuffer vertexBuffer;
			VmaAllocation vertexAllocation;

			// Index Buffer
			VkBuffer indexBuffer;
			VmaAllocation indexAllocation;

			// Uniform Buffers
			std::vector<VkBuffer> uniformBuffers;
			std::vector<VmaAllocation> uniformAllocations;

			std::vector<VkDescriptorSet> descriptorSets;
		};
	}
}