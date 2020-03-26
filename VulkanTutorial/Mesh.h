#pragma once

#include <vulkan/vulkan.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <vector>
#include <array>

#include "Texture.h"

struct MeshMatrices
{
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 inv_model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};

struct Transform
{
	glm::vec3 position;
	glm::vec3 rotation;
	glm::vec3 scale;
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
};

namespace std
{
	template<> struct hash<Vertex>
	{
		size_t operator()(Vertex const& vertex) const
		{
			return ((hash<glm::vec3>()(vertex.pos) ^
				(hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
				(hash<glm::vec2>()(vertex.texCoord) << 1);
		}
	};
};

struct MeshData
{
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
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

class Mesh
{
public:

	Mesh();
	~Mesh();

	void SetupMesh(std::vector<Vertex> vertices_, std::vector<uint32_t> indices_);

	void SetColor(glm::vec3 color_);

	//inline Texture& GetTexture() { return texture; };
	//inline void SetTexture(Texture texture_) { texture = texture_; };

	inline std::vector<Vertex> GetVertices() { return meshData.vertices; };
	inline std::vector<uint32_t> GetIndices() { return meshData.indices; };

	/*inline VkBuffer& GetVertexBuffer() { return vertexBuffer; };
	inline VkBuffer& GetIndexBuffer() { return indexBuffer; };
	inline VkBuffer& GetUniformBuffer(int i) { return uniformBuffers[i]; };
	
	inline VkDeviceMemory& GetVertexMemory() { return vertexBufferMemory; };
	inline VkDeviceMemory& GetIndexMemory() { return indexBufferMemory; };
	inline VkDeviceMemory& GetUniformBufferMemory(int i) { return uniformBuffersMemory[i]; };
	
	inline std::vector<VkDescriptorSet>& GetDescriptorSets() { return descriptorSets; };
	inline std::vector<VkBuffer>& GetUniformBufferVector() { return uniformBuffers; };
	inline std::vector<VkDeviceMemory>& GetUniformMemoryVector() { return uniformBuffersMemory; };*/
	

	inline MeshMatrices GetMatrices() { return matrices; };
	inline void SetMatrices(MeshMatrices matrices_) { matrices = matrices_; };

	
	inline Transform GetTransform() { return transform; };
	inline void SetTransform(Transform transform_) { transform = transform_; };
	inline void SetTransform(glm::vec3 position, glm::vec3 rotation, glm::vec3 scale) 
	{ 
		transform.position = position; 
		transform.rotation = rotation; 
		transform.scale = scale; 
	};

private:

	MeshMatrices matrices;
	Transform transform;
	MeshData meshData;
	//Texture texture;

	//// Vertex Buffer
	//VkBuffer vertexBuffer;
	//VkDeviceMemory vertexBufferMemory;

	//// Index Buffer
	//VkBuffer indexBuffer;
	//VkDeviceMemory indexBufferMemory;

	//// Uniform Buffers
	//std::vector<VkBuffer> uniformBuffers;
	//std::vector<VkDeviceMemory> uniformBuffersMemory;

	//std::vector<VkDescriptorSet> descriptorSets;
};