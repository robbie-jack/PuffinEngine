#pragma once

#ifndef MESH_COMPONENT_H
#define MESH_COMPONENT_H

#include <vulkan/vulkan.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <Components/BaseComponent.h>

#include <vector>
#include <array>

#include <Rendering/VKTypes.h>

#include <cereal/types/string.hpp>

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
			};

			template<class Archive>
			void serialize(Archive& archive)
			{
				archive(pos.x, pos.y, pos.z);
				archive(color.x, color.y, color.z);
				archive(normal.x, normal.y, normal.z);
				archive(tangent.x, tangent.y, tangent.z);
				archive(uv.x, uv.y);
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
		struct MeshComponent : public BaseComponent
		{
			MeshComponent()
			{
				bFlagCreated = true;
				bFlagDeleted = false;
			}

			MeshComponent(std::string InModelPath, std::string InTexturePath) :
				model_path(InModelPath), texture_path(InTexturePath)
			{
				MeshComponent();
			}

			// Mesh Data
			std::vector<Vertex> vertices;
			std::vector<uint32_t> indices;
			std::string model_path;

			// Texture
			Texture texture;
			std::string texture_path;

			// Material
			Material material;

			// Vertex Buffer
			AllocatedBuffer vertexBuffer;

			// Index Buffer
			AllocatedBuffer indexBuffer;
		};

		template<class Archive>
		void serialize(Archive& archive, MeshComponent& comp)
		{
			archive(comp.model_path, comp.texture_path);
		}
	}
}

#endif // !MESH_COMPONENT_H