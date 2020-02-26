#pragma once

#include <vulkan/vulkan.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>
#include <array>

#include "Texture.h"

struct Matrices
{
	alignas(16) glm::mat4 model;
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

	static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions()
	{
		std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions = {};

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
		attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

		return attributeDescriptions;
	};
};

class Mesh
{
public:

	Mesh();
	~Mesh();

	void SetupMesh(std::vector<Vertex> vertices_, std::vector<uint32_t> indices_);
	void Cleanup(VkDevice device);

	void BuildTransform();

	inline Texture& GetTexture() { return texture; };

	inline std::vector<Vertex> GetVertices() { return vertices; };
	inline std::vector<uint32_t> GetIndices() { return indices; };

	inline VkBuffer& GetVertexBuffer() { return vertexBuffer; };
	inline VkBuffer& GetIndexBuffer() { return indexBuffer; };
	inline VkBuffer& GetUniformBuffer(int i) { return uniformBuffers[i]; };
	
	inline VkDeviceMemory& GetVertexMemory() { return vertexBufferMemory; };
	inline VkDeviceMemory& GetIndexMemory() { return indexBufferMemory; };
	inline VkDeviceMemory& GetUniformBufferMemory(int i) { return uniformBuffersMemory[i]; };
	
	inline std::vector<VkDescriptorSet>& GetDescriptorSets() { return descriptorSets; };
	inline std::vector<VkBuffer>& GetUniformBufferVector() { return uniformBuffers; };
	inline std::vector<VkDeviceMemory>& GetUniformMemoryVector() { return uniformBuffersMemory; };

	inline Matrices GetMatrices() { return matrices; };
	inline void SetMatrices(Matrices matrices_) { matrices = matrices_; };

	inline Transform GetTransform() { return transform; };
	inline void SetTransform(Transform transform_) { transform = transform_; };
	inline void SetTransform(glm::vec3 position, glm::vec3 rotation, glm::vec3 scale) 
	{ 
		transform.position = position; 
		transform.rotation = rotation; 
		transform.scale = scale; 
	};

private:

	Texture texture;

	std::vector<Vertex> vertices;
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

	Matrices matrices;
	Transform transform;
};