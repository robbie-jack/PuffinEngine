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
#include "VKTypes.h"

namespace Puffin
{
	namespace Rendering
	{
		struct LightData
		{
			alignas(16) glm::vec3 position;
			alignas(16) glm::vec3 ambientColor;
			alignas(16) glm::vec3 diffuseColor;
			alignas(4) float specularStrength;
			alignas(4) int shininess;
		};

		template<class Archive>
		void serialize(Archive& archive, LightData& data)
		{
			archive(data.ambientColor.x, data.ambientColor.y, data.ambientColor.z);
			archive(data.diffuseColor.x, data.diffuseColor.y, data.diffuseColor.z);
			archive(data.specularStrength, data.shininess);
		}

		struct LightComponent : public BaseComponent
		{
			LightData data;
			std::vector<AllocatedBuffer> buffers;
		};

		template<class Archive>
		void serialize(Archive& archive, LightComponent& comp)
		{
			archive(comp.data);
		}
	}
}

#endif // LIGHT_COMPONENT_H