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
		struct GPUPointLightData
		{
			alignas(16) glm::vec3 ambientColor;
			alignas(16) glm::vec3 diffuseColor;

			alignas(16) glm::vec3 position;

			alignas(4) float constant;
			alignas(4) float linear;
			alignas(4) float quadratic;

			alignas(4) float specularStrength;
			alignas(4) int shininess;

			alignas(4) int shadowmapIndex;
		};

		struct GPUDirLightData
		{
			alignas(16) glm::vec3 ambientColor;
			alignas(16) glm::vec3 diffuseColor;

			alignas(16) glm::vec3 direction;

			alignas(4) float specularStrength;
			alignas(4) int shininess;

			alignas(4) int shadowmapIndex;
		};

		struct GPUSpotLightData
		{
			alignas(16) glm::vec3 ambientColor;
			alignas(16) glm::vec3 diffuseColor;

			alignas(16) glm::vec3 position;
			alignas(16) glm::vec3 direction;

			alignas(4) float innerCutoff;
			alignas(4) float outerCutoff;

			alignas(4) float constant;
			alignas(4) float linear;
			alignas(4) float quadratic;

			alignas(4) float specularStrength;
			alignas(4) int shininess;

			alignas(4) int shadowmapIndex;
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
			LightType type;
			Vector3f ambientColor, diffuseColor;
			Vector3f direction;
			float specularStrength;
			int shininess;
			float constantAttenuation, linearAttenuation, quadraticAttenuation; // USed to calculate light dropoff based on distance
			float innerCutoffAngle, outerCutoffAngle; // Used for spotlight

			// Variables for computing shadows cast by lights
			bool bFlagCastShadows; // Flag to indicate if light should cast shadows
			std::vector<AllocatedImage> depthAttachments;
			std::vector<VkFramebuffer> depthFramebuffers;
			glm::mat4 lightSpaceView;

			NLOHMANN_DEFINE_TYPE_INTRUSIVE(LightComponent, type, ambientColor, diffuseColor, direction,
				specularStrength, shininess, constantAttenuation, linearAttenuation, quadraticAttenuation,
				innerCutoffAngle, outerCutoffAngle, bFlagCastShadows)
		};
	}
}