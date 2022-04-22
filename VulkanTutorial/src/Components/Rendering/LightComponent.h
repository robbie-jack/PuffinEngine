#pragma once

#ifndef LIGHT_COMPONENT_H
#define LIGHT_COMPONENT_H

#include <vulkan/vulkan.h>
#include <Rendering/vk_mem_alloc.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <Types/Vector.h>

#include <vector>
#include <Rendering/VKTypes.h>

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
		};

		template<class Archive>
		void save(Archive& archive, const LightComponent& comp)
		{
			int lightType = (int)comp.type;

			archive(lightType);
			archive(comp.ambientColor.x, comp.ambientColor.y, comp.ambientColor.z);
			archive(comp.diffuseColor.x, comp.diffuseColor.y, comp.diffuseColor.z);
			archive(comp.direction.x, comp.direction.y, comp.direction.z);
			archive(comp.specularStrength, comp.shininess);
			archive(comp.constantAttenuation, comp.linearAttenuation, comp.quadraticAttenuation);
			archive(comp.innerCutoffAngle, comp.outerCutoffAngle);
			archive(comp.bFlagCastShadows);
		}

		template<class Archive>
		void load(Archive& archive, LightComponent& comp)
		{
			int lightType;

			archive(lightType);
			archive(comp.ambientColor.x, comp.ambientColor.y, comp.ambientColor.z);
			archive(comp.diffuseColor.x, comp.diffuseColor.y, comp.diffuseColor.z);
			archive(comp.direction.x, comp.direction.y, comp.direction.z);
			archive(comp.specularStrength, comp.shininess);
			archive(comp.constantAttenuation, comp.linearAttenuation, comp.quadraticAttenuation);
			archive(comp.innerCutoffAngle, comp.outerCutoffAngle);
			archive(comp.bFlagCastShadows);

			comp.type = (LightType)lightType;
		}
	}
}

#endif // LIGHT_COMPONENT_H