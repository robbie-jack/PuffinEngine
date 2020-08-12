#pragma once

#include <vulkan/vulkan.h>
#include "vk_mem_alloc.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>

namespace Puffin
{
	namespace Rendering
	{
		struct LightBufferObject
		{
			alignas(16) glm::vec3 position;
			alignas(16) glm::vec3 ambientColor;
			alignas(16) glm::vec3 diffuseColor;
			alignas(4) float specularStrength;
			alignas(4) int shininess;
		};

		class Light
		{
		public:

			Light();
			~Light();

			void InitLight(glm::vec3 position, glm::vec3 ambient, glm::vec3 diffuse, float specular = 0.5f, int shininess = 32);

			inline glm::vec3 GetLightPosition() { return lightUniformBuffer.position; };
			inline void SetLightPosition(glm::vec3 position) { lightUniformBuffer.position = position; };

			inline LightBufferObject GetLightBufferObject() { return lightUniformBuffer; };

			inline VkBuffer& GetLightBuffer(int i) { return lightBufferVector[i]; };
			inline VmaAllocation& GetLightAllocation(int i) { return lightAllocationVector[i]; };

			inline std::vector<VkBuffer>& GetLightBufferVector() { return lightBufferVector; };
			inline std::vector<VmaAllocation>& GetLightAllocationVector() { return lightAllocationVector; };

		private:

			LightBufferObject lightUniformBuffer;

			std::vector<VkBuffer> lightBufferVector;
			std::vector<VmaAllocation> lightAllocationVector;
		};
	}
}