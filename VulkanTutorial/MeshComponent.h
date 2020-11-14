#pragma once

#include "Mesh.h"
#include "Texture.h"

#include <vector>

namespace Puffin
{
	namespace Rendering
	{
		struct MeshComponent
		{
			// Mesh Data
			MeshMatrices matrices;
			std::vector<Vertex> vertices;
			std::vector<uint32_t> indices;

			// Texture
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