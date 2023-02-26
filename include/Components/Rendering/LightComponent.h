#pragma once

#include <vulkan/vulkan.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Types/Vector.h"

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

		const std::vector<std::string> LIGHT_TYPE_LABELS = { "Point", "Spot", "Directional" };

		NLOHMANN_JSON_SERIALIZE_ENUM(LightType, {
			{ LightType::POINT, "Point"},
			{ LightType::SPOT, "Spot"},
			{ LightType::DIRECTIONAL, "Directional"}
		})

		struct LightComponent
		{
			Vector3f color = {1.f, 1.f, 1.f};
			Vector3f direction = { .5f, -.5f, 0.f };

			float ambientIntensity = .05f; // Intensity multiplier applied to ambient/indirect color
			float specularIntensity = 1.f; // Intensity multiplier applied to specular highlights
			int specularExponent = 64; // Exponent specular value is raised to

			float constantAttenuation = 1.f;
			float linearAttenuation = .09f;
			float quadraticAttenuation = .032f;

			float innerCutoffAngle = 30.0f;
			float outerCutoffAngle = 45.0f;

			LightType type = LightType::POINT;

			NLOHMANN_DEFINE_TYPE_INTRUSIVE(LightComponent, 
				color, 
				direction,
				ambientIntensity,
				specularIntensity, 
				specularExponent, 
				constantAttenuation, 
				linearAttenuation, 
				quadraticAttenuation,
				innerCutoffAngle,
				outerCutoffAngle,
				type)
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