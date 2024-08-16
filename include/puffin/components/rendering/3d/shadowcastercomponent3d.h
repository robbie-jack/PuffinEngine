#pragma once

#include <json.hpp>
#include <string>
#include <vector>
#include <glm/fwd.hpp>
#include <glm/ext/matrix_transform.hpp>

#include "puffin/rendering/resourceid.h"
#include "puffin/types/aabb.h"

namespace puffin::rendering
{
	const std::vector<uint16_t> gShadowResolutionValues = { 512, 1024, 2048, 4096, 8192, 16384 };
	const std::vector<std::string> gShadowResolutionLabels = { "512", "1024", "2048", "4096", "8192", "16384" };

	struct ShadowCascade
	{
		glm::mat4 lightView = glm::identity<glm::mat4>();
		glm::mat4 lightViewProj = glm::identity<glm::mat4>();
		float planeDistance = 0.0f;
	};

	// Component for lights that cast shadows
	struct ShadowCasterComponent
	{
		ShadowCasterComponent() = default;

		uint16_t width = 2048;
		uint16_t height = 2048;
		float biasMin = 0.05f;
		float biasMax = 0.5f;
		float boundsMult = 2.0f;
		int cascadeCount = 1;

		glm::mat4 lightView = glm::identity<glm::mat4>();
		glm::mat4 lightViewProj = glm::identity<glm::mat4>();
		int shadowIdx = -1;
		ResourceID resourceID = gInvalidID;

		AABB_3D boundsAABB;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(ShadowCasterComponent, width, height, biasMin, biasMax, boundsMult, cascadeCount)
	};
}