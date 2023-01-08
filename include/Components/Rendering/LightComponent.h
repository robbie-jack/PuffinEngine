#pragma once

#include <vulkan/vulkan.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Types/Vector.h"
#include "Rendering/Vulkan/VKTypes.h"

#include <vector>
#include <string>

#include "nlohmann/json.hpp"

namespace Puffin
{
	namespace Rendering
	{
		enum class LightType
		{
			POINT = 0,
			SPOT = 1,
			DIRECTIONAL = 2
		};

		NLOHMANN_JSON_SERIALIZE_ENUM(LightType, {
			{ LightType::POINT, "Point"},
			{ LightType::SPOT, "Spot"},
			{ LightType::DIRECTIONAL, "Directional"}
		})

		struct LightComponent
		{
			Vector3f ambientColor = {.1f, .1f, .1f};
			Vector3f diffuseColor = {1.f, 1.f, 1.f};
			float specularStrength = .5f;
			int shininess = 16;
		};

		struct PointLightComponent : public LightComponent
		{
			float constantAttenuation = 1.f;
			float linearAttenuation = .09f;
			float quadraticAttenuation = .032f;

			NLOHMANN_DEFINE_TYPE_INTRUSIVE(PointLightComponent, ambientColor, diffuseColor, specularStrength, shininess, constantAttenuation, linearAttenuation, quadraticAttenuation)
		};

		struct DirectionalLightComponent : public LightComponent
		{
			Vector3f direction = {.5f, -.5f, 0.f};

			NLOHMANN_DEFINE_TYPE_INTRUSIVE(DirectionalLightComponent, ambientColor, diffuseColor, specularStrength, shininess, direction)
		};

		struct SpotLightComponent : public PointLightComponent
		{
			Vector3f direction = { .5f, .5f, 0.f };

			float innerCutoffAngle = 30.0f;
			float outerCutoffAngle = 45.0f;

			NLOHMANN_DEFINE_TYPE_INTRUSIVE(SpotLightComponent, ambientColor, diffuseColor, specularStrength, shininess, direction, constantAttenuation, linearAttenuation, quadraticAttenuation, innerCutoffAngle, outerCutoffAngle)
		};

		const std::vector<uint16_t> SHADOW_RESOLUTION_VALUES = { 512, 1024, 2048, 4096 };
		const std::vector<std::string> SHADOW_RESOLUTION_LABELS = { "512", "1024", "2048", "4096" };

		// Component for lights that cast shadows
		struct ShadowCasterComponent
		{
			uint16_t shadowmapWidth = 2048;
			uint16_t shadowmapHeight = 2048;
			glm::mat4 lightSpaceView;

			NLOHMANN_DEFINE_TYPE_INTRUSIVE(ShadowCasterComponent, shadowmapWidth, shadowmapHeight)
		};
	}
}