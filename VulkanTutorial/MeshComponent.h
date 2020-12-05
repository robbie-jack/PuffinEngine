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

#include <vector>
#include <array>

#include "BaseComponent.h"
#include "VKTypes.h"

#include <cereal/types/string.hpp>

namespace Puffin
{
	namespace Rendering
	{
		struct MeshMatrices
		{
			alignas(16) glm::mat4 model;
			alignas(16) glm::mat4 inv_model;
			alignas(16) glm::mat4 view;
			alignas(16) glm::mat4 proj;
		};

		struct Vertex
		{
			glm::vec3 pos;
			glm::vec3 normal;
			glm::vec3 color;
			glm::vec2 texCoord;

			bool operator==(const Vertex& other) const
			{
				return pos == other.pos && color == other.color && texCoord == other.texCoord;
			}

			static VkVertexInputBindingDescription getBindingDescription()
			{
				VkVertexInputBindingDescription bindingDescription = {};
				bindingDescription.binding = 0;
				bindingDescription.stride = sizeof(Vertex);
				bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

				return bindingDescription;
			}

			static std::array<VkVertexInputAttributeDescription, 4> getAttributeDescriptions()
			{
				std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions = {};

				attributeDescriptions[0].binding = 0;
				attributeDescriptions[0].location = 0;
				attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
				attributeDescriptions[0].offset = offsetof(Vertex, pos);

				attributeDescriptions[1].binding = 0;
				attributeDescriptions[1].location = 1;
				attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
				attributeDescriptions[1].offset = offsetof(Vertex, normal);

				attributeDescriptions[2].binding = 0;
				attributeDescriptions[2].location = 2;
				attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
				attributeDescriptions[2].offset = offsetof(Vertex, color);

				attributeDescriptions[3].binding = 0;
				attributeDescriptions[3].location = 3;
				attributeDescriptions[3].format = VK_FORMAT_R32G32_SFLOAT;
				attributeDescriptions[3].offset = offsetof(Vertex, texCoord);

				return attributeDescriptions;
			};

			template<class Archive>
			void serialize(Archive& archive)
			{
				archive(pos.x, pos.y, pos.z);
				archive(normal.x, normal.y, normal.z);
				archive(color.x, color.y, color.z);
				archive(texCoord.x, texCoord.y);
			}
		};

		const std::vector<Vertex> cube_vertices =
		{
			// Front
			{{-0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}}, // 0
			{{-0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}}, // 1
			{{0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}, // 2
			{{0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}, // 3

			// Left
			{{-0.5f, 0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}, // 4
			{{-0.5f, -0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}, // 5
			{{-0.5f, -0.5f, 0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}}, // 6
			{{-0.5f, 0.5f, 0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}}, // 7

			// Back
			{{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}}, // 9
			{{0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}}, // 9
			{{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}, // 10
			{{-0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}, // 11

			// Right
			{{0.5f, 0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}, // 12
			{{0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}, // 13
			{{0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}}, // 14
			{{0.5f, 0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}}, // 15

			// Top
			{{-0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}, // 16
			{{-0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}}, // 17
			{{0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}, // 18
			{{0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}}, // 19

			// Bottom
			{{-0.5f, -0.5f, 0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}}, // 20
			{{-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}, // 21
			{{0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}}, // 22
			{{0.5f, -0.5f, 0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}} // 23
		};

		const std::vector<uint32_t> cube_indices =
		{
			0, 1, 2, // Front
			0, 2, 3,

			4, 5, 6, // Left
			4, 6, 7,

			8, 9, 10, // Back
			8, 10, 11,

			12, 13, 14, // Right
			12, 14, 15,

			16, 17, 18, // Top
			16, 18, 19,

			20, 21, 22, // Bottom
			20, 22, 23
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
				(hash<glm::vec2>()(vertex.texCoord) << 1);
		}
	};
};

namespace Puffin
{
	namespace Rendering
	{
		struct MeshComponent : public BaseComponent
		{
			// Mesh Data
			MeshMatrices matrices;
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

			// Uniform Buffers
			std::vector<AllocatedBuffer> uniformBuffers;

			std::vector<VkDescriptorSet> descriptorSets;
		};

		template<class Archive>
		void serialize(Archive& archive, MeshComponent& comp)
		{
			archive(comp.model_path, comp.texture_path);
		}
	}
}

#endif // !MESH_COMPONENT_H