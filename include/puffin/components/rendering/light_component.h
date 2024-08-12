#pragma once

#include <vector>
#include <string>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include "puffin/types/vector.h"
#include "puffin/rendering/resource_id.h"

#include "nlohmann/json.hpp"
#include "puffin/types/aabb.h"

namespace puffin
{
	namespace rendering
	{
		enum class LightType
		{
			Point = 0,
			Spot = 1,
			Directional = 2
		};

		const std::vector<std::string> gLightTypeLabels = { "Point", "Spot", "Directional" };

		NLOHMANN_JSON_SERIALIZE_ENUM(LightType, {
			{ LightType::Point, "Point"},
			{ LightType::Spot, "Spot"},
			{ LightType::Directional, "Directional"}
		})

		struct LightComponent
		{
			Vector3f color = {1.f, 1.f, 1.f};
			Vector3f direction = { 1.0f, 0.0f, 0.0f };

			float ambient_intensity = .05f; // Intensity multiplier applied to ambient/indirect color
			float specular_intensity = 1.f; // Intensity multiplier applied to specular highlights
			int specular_exponent = 64; // Exponent specular value is raised to

			float constant_attenuation = 1.f;
			float linear_attenuation = .09f;
			float quadratic_attenuation = .032f;

			float inner_cutoff_angle = 30.0f;
			float outer_cutoff_angle = 45.0f;

			LightType type = LightType::Point;

			NLOHMANN_DEFINE_TYPE_INTRUSIVE(LightComponent, 
				color,
				ambient_intensity,
				specular_intensity, 
				specular_exponent, 
				constant_attenuation, 
				linear_attenuation, 
				quadratic_attenuation,
				inner_cutoff_angle,
				outer_cutoff_angle,
				type)
		};

		const std::vector<uint16_t> g_shadow_resolution_values = { 512, 1024, 2048, 4096, 8192, 16384 };
		const std::vector<std::string> g_shadow_resolution_labels = { "512", "1024", "2048", "4096", "8192", "16384" };

        struct ShadowCascade
        {
            glm::mat4 light_view = glm::identity<glm::mat4>();
            glm::mat4 light_view_proj = glm::identity<glm::mat4>();
            float plane_distance = 0.0f;
        };

		// Component for lights that cast shadows
		struct ShadowCasterComponent
		{
			ShadowCasterComponent() = default;

			uint16_t width = 2048;
			uint16_t height = 2048;
			float bias_min = 0.05f;
			float bias_max = 0.5f;
			float bounds_mult = 2.0f;
            int cascade_count = 1;

			glm::mat4 light_view = glm::identity<glm::mat4>();
			glm::mat4 light_view_proj = glm::identity<glm::mat4>();
			int shadow_idx = -1;
			ResourceID resource_id = gInvalidID;

			AABB_3D bounds_aabb;

			NLOHMANN_DEFINE_TYPE_INTRUSIVE(ShadowCasterComponent, width, height, bias_min, bias_max, bounds_mult, cascade_count)
		};
	}
}
