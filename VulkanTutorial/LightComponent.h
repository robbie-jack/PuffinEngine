#pragma once

#include <vulkan/vulkan.h>
#include "vk_mem_alloc.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>
#include "BaseComponent.h"

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

		struct LightComponent : public BaseComponent
		{
			LightBufferObject uniformBuffer;
			std::vector<VkBuffer> buffers;
			std::vector<VmaAllocation> allocations;
		};
	}
}