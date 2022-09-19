#pragma once

#include <vulkan/vulkan.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <Types/Vector.h>

#include <vector>
#include <Rendering/VKTypes.h>

#include "nlohmann/json.hpp"

namespace Puffin
{
	namespace Rendering
	{
		struct GPULightData
		{
			alignas(16) glm::vec3 ambientColor;
			alignas(16) glm::vec3 diffuseColor;

			alignas(4) float specularStrength;
			alignas(4) int shininess;

			alignas(16) glm::mat4 lightSpaceMatrix;
			alignas(4) int shadowmapIndex;
		};

		struct GPUPointLightData
		{
			alignas(16) glm::vec3 position;

			alignas(4) float constant;
			alignas(4) float linear;
			alignas(4) float quadratic;

			alignas(4) int dataIndex;
		};

		struct GPUDirLightData
		{
			alignas(16) glm::vec3 direction;

			alignas(4) int dataIndex;
		};

		struct GPUSpotLightData
		{
			alignas(16) glm::vec3 position;
			alignas(16) glm::vec3 direction;

			alignas(4) float innerCutoff;
			alignas(4) float outerCutoff;

			alignas(4) float constant;
			alignas(4) float linear;
			alignas(4) float quadratic;

			alignas(4) int dataIndex;
		};

		struct GPULightStatsData
		{
			alignas(4) int numPLights;
			alignas(4) int numDLights;
			alignas(4) int numSLights;
		};

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
			Vector3f direction = {.5f, .5f, 0.f};

			NLOHMANN_DEFINE_TYPE_INTRUSIVE(DirectionalLightComponent, ambientColor, diffuseColor, specularStrength, shininess, direction)
		};

		struct SpotLightComponent : public PointLightComponent
		{
			Vector3f direction = { .5f, .5f, 0.f };

			float innerCutoffAngle = 12.5f;
			float outerCutoffAngle = 17.5f;

			NLOHMANN_DEFINE_TYPE_INTRUSIVE(SpotLightComponent, ambientColor, diffuseColor, specularStrength, shininess, direction, constantAttenuation, linearAttenuation, quadraticAttenuation, innerCutoffAngle, outerCutoffAngle)
		};

		// Component for lights that cast shadows
		struct ShadowCasterComponent
		{
			uint32_t shadowmapWidth = 2048;
			uint32_t shadowmapHeight = 2048;
			glm::mat4 lightSpaceView;

			NLOHMANN_DEFINE_TYPE_INTRUSIVE(ShadowCasterComponent, shadowmapWidth, shadowmapHeight)
		};
	}
}