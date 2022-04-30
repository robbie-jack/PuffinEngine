#pragma once

#include <Rendering/VKTypes.h>
#include <Types/UUID.h>

#include <vulkan/vulkan.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <vector>
#include <array>
#include <filesystem>

#include "nlohmann/json.hpp"

namespace fs = std::filesystem;

namespace Puffin
{
	namespace Rendering
	{
		struct Vertex
		{
			glm::vec3 pos;
			glm::vec3 color;
			glm::vec3 normal;
			glm::vec3 tangent;
			glm::vec2 uv;

			bool operator==(const Vertex& other) const
			{
				return pos == other.pos
					&& color == other.color
					&& normal == other.normal
					&& tangent == other.tangent
					&& uv == other.uv;
			}

			static VkVertexInputBindingDescription getBindingDescription()
			{
				VkVertexInputBindingDescription bindingDescription = {};
				bindingDescription.binding = 0;
				bindingDescription.stride = sizeof(Vertex);
				bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

				return bindingDescription;
			}

			static std::array<VkVertexInputAttributeDescription, 5> getAttributeDescriptions()
			{
				std::array<VkVertexInputAttributeDescription, 5> attributeDescriptions = {};

				attributeDescriptions[0].binding = 0;
				attributeDescriptions[0].location = 0;
				attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
				attributeDescriptions[0].offset = offsetof(Vertex, pos);

				attributeDescriptions[1].binding = 0;
				attributeDescriptions[1].location = 1;
				attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
				attributeDescriptions[1].offset = offsetof(Vertex, color);

				attributeDescriptions[2].binding = 0;
				attributeDescriptions[2].location = 2;
				attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
				attributeDescriptions[2].offset = offsetof(Vertex, normal);

				attributeDescriptions[3].binding = 0;
				attributeDescriptions[3].location = 3;
				attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
				attributeDescriptions[3].offset = offsetof(Vertex, tangent);

				attributeDescriptions[4].binding = 0;
				attributeDescriptions[4].location = 4;
				attributeDescriptions[4].format = VK_FORMAT_R32G32_SFLOAT;
				attributeDescriptions[4].offset = offsetof(Vertex, uv);

				return attributeDescriptions;
			}
		};
	}
}

namespace std
{
	template<> struct hash<Puffin::Rendering::Vertex>
	{
		size_t operator()(Puffin::Rendering::Vertex const& vertex) const
		{
			return ((hash<glm::vec3>()(vertex.pos) ^
				(hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
				(hash<glm::vec3>()(vertex.normal) << 1) ^
				(hash<glm::vec3>()(vertex.tangent) << 1) ^
				(hash<glm::vec2>()(vertex.uv) << 1);
		}
	};
};

namespace Puffin
{
	namespace Rendering
	{
		struct MeshComponent
		{
			MeshComponent() {}
			
			MeshComponent(UUID InMeshID, UUID InTextureID) :
				meshAssetID(InMeshID), textureAssetID(InTextureID)
			{
			}

			// Mesh Data
			UUID meshAssetID;
			uint32_t vertexCount;
			uint32_t indexCount;

			// Texture
			Texture texture;
			UUID textureAssetID;

			// Material
			Material material;

			// Vertex Buffer
			AllocatedBuffer vertexBuffer;

			// Index Buffer
			AllocatedBuffer indexBuffer;

			NLOHMANN_DEFINE_TYPE_INTRUSIVE(MeshComponent, meshAssetID, textureAssetID)
		};
	}
}