#pragma once

#include "nlohmann/json.hpp"

#include "puffin/components/rendering/3d/lightcomponent3d.h"
#include "puffin/utility/reflection.h"

namespace puffin
{
	namespace rendering
	{
		/*
		 * Component containing variables used by point lights
		 */
		struct PointLightComponent3D : LightComponent3D
		{
			float constantAttenuation = 1.f;
			float linearAttenuation = .09f;
			float quadraticAttenuation = .032f;

			NLOHMANN_DEFINE_TYPE_INTRUSIVE(PointLightComponent3D, color, ambientIntensity, specularIntensity, specularExponent,
				constantAttenuation, linearAttenuation, quadraticAttenuation)
		};
	}

	template<>
	inline void reflection::RegisterType<rendering::PointLightComponent3D>()
	{
		using namespace rendering;

		entt::meta<PointLightComponent3D>()
			.type(entt::hs("PointLightComponent3D"))
			.data<&PointLightComponent3D::color>(entt::hs("color"))
			.data<&PointLightComponent3D::ambientIntensity>(entt::hs("ambientIntensity"))
			.data<&PointLightComponent3D::specularIntensity>(entt::hs("specularIntensity"))
			.data<&PointLightComponent3D::specularExponent>(entt::hs("specularExponent"))
			.data<&PointLightComponent3D::constantAttenuation>(entt::hs("constantAttenuation"))
			.data<&PointLightComponent3D::linearAttenuation>(entt::hs("linearAttenuation"))
			.data<&PointLightComponent3D::quadraticAttenuation>(entt::hs("quadraticAttenuation"));
	}
}