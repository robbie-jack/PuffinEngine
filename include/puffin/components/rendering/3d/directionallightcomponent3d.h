#pragma once

#include "nlohmann/json.hpp"

#include "puffin/components/rendering/3d/lightcomponent3d.h"
#include "puffin/utility/reflection.h"

namespace puffin
{
	namespace rendering
	{
		/*
		 * Component containing variables used by directional lights
		 */
		struct DirectionalLightComponent3D : LightComponent3D
		{
			Vector3f direction = { 1.0f, 0.0f, 0.0f };

			NLOHMANN_DEFINE_TYPE_INTRUSIVE(DirectionalLightComponent3D, color, ambientIntensity, specularIntensity, specularExponent)
		};
	}

	template<>
	inline void reflection::RegisterType<rendering::DirectionalLightComponent3D>()
	{
		using namespace rendering;

		entt::meta<DirectionalLightComponent3D>()
			.type(entt::hs("DirectionalLightComponent3D"))
			.data<&DirectionalLightComponent3D::color>(entt::hs("color"))
			.data<&DirectionalLightComponent3D::ambientIntensity>(entt::hs("ambientIntensity"))
			.data<&DirectionalLightComponent3D::specularIntensity>(entt::hs("specularIntensity"))
			.data<&DirectionalLightComponent3D::specularExponent>(entt::hs("specularExponent"));
	}
}
