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

		enum class LightType
		{
			POINT = 0,
			SPOT = 1,
			DIRECTIONAL = 2
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
			LightType type;
		};

		/*template<class Archive>
		void serialize(Archive& archive, LightComponent& comp)
		{
			archive(comp.data, comp.type);
		}*/

		template<class Archive>
		void save(Archive& archive, const LightComponent& comp)
		{
			int lightType = (int)comp.type;

			archive(comp.data, lightType);
		}

		template<class Archive>
		void load(Archive& archive, LightComponent& comp)
		{
			int lightType;

			archive(comp.data, lightType);

			comp.type = (LightType)lightType;
		}
	}
}

#endif // LIGHT_COMPONENT_H