#pragma once

#include <json.hpp>
#include <string>
#include <vector>
#include <glm/fwd.hpp>
#include <glm/ext/matrix_transform.hpp>

#include "puffin/rendering/resourceid.h"
#include "puffin/types/aabb.h"
#include "puffin/utility/reflection.h"

namespace puffin
{
	namespace rendering
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
		struct ShadowCasterComponent3D
		{
			ShadowCasterComponent3D() = default;

			uint16_t width = 2048;
			uint16_t height = 2048;
			float biasMin = 0.05f;
			float biasMax = 0.5f;
			float boundsMult = 2.0f;
			int cascadeCount = 1;

			glm::mat4 lightView = glm::identity<glm::mat4>();
			glm::mat4 lightViewProj = glm::identity<glm::mat4>();
			int shadowIdx = -1;
			std::vector<ResourceID> shadowCascadeIDs;

			AABB3D boundsAABB;

			NLOHMANN_DEFINE_TYPE_INTRUSIVE(ShadowCasterComponent3D, width, height, biasMin, biasMax, boundsMult, cascadeCount)
		};
	}

	template<>
	inline void reflection::RegisterType<rendering::ShadowCasterComponent3D>()
	{
		using namespace rendering;

		entt::meta<ShadowCasterComponent3D>()
			.type(entt::hs("ShadowCasterComponent3D"))
			.data<&ShadowCasterComponent3D::width>(entt::hs("width"))
			.data<&ShadowCasterComponent3D::height>(entt::hs("height"))
			.data<&ShadowCasterComponent3D::biasMin>(entt::hs("biasMin"))
			.data<&ShadowCasterComponent3D::biasMax>(entt::hs("biasMax"))
			.data<&ShadowCasterComponent3D::boundsMult>(entt::hs("boundsMult"))
			.data<&ShadowCasterComponent3D::cascadeCount>(entt::hs("cascadeCount"));
	}

	namespace serialization
	{
		template<>
		inline void Serialize<rendering::ShadowCasterComponent3D>(const rendering::ShadowCasterComponent3D& data, Archive& archive)
		{
			archive.Set("width", data.width);
			archive.Set("height", data.height);
			archive.Set("biasMin", data.biasMin);
			archive.Set("biasMax", data.biasMax);
			archive.Set("boundsMult", data.boundsMult);
			archive.Set("cascadeCount", data.cascadeCount);
		}

		template<>
		inline void Deserialize<rendering::ShadowCasterComponent3D>(const Archive& archive, rendering::ShadowCasterComponent3D& data)
		{
			archive.Get("width", data.width);
			archive.Get("height", data.height);
			archive.Get("biasMin", data.biasMin);
			archive.Get("biasMax", data.biasMax);
			archive.Get("boundsMult", data.boundsMult);
			archive.Get("cascadeCount", data.cascadeCount);
		}
	}
}