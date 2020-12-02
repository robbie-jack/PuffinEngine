#pragma once

#ifndef LIGHT_COMPONENT_H
#define LIGHT_COMPONENT_H

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

		template<class Archive>
		void serialize(Archive& archive, LightBufferObject& buffer)
		{
			archive(buffer.ambientColor.x, buffer.ambientColor.y, buffer.ambientColor.z);
			archive(buffer.diffuseColor.x, buffer.diffuseColor.y, buffer.diffuseColor.z);
			archive(buffer.specularStrength, buffer.shininess);
		}

		struct LightComponent : public BaseComponent
		{
			LightBufferObject uniformBuffer;
			std::vector<VkBuffer> buffers;
			std::vector<VmaAllocation> allocations;
		};

		template<class Archive>
		void serialize(Archive& archive, LightComponent& comp)
		{
			archive(comp.uniformBuffer);
		}
	}
}

#endif // LIGHT_COMPONENT_H