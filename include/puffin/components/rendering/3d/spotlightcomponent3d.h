#pragma once

#include "nlohmann/json.hpp"

#include "puffin/components/rendering/3d/pointlightcomponent3d.h"
#include "puffin/utility/reflection.h"
#include "puffin/utility/serialization.h"

namespace puffin
{
	namespace rendering
	{
		/*
		 * Component containing variables used by spot lights
		 */
		struct SpotLightComponent3D : PointLightComponent3D
		{
			float innerCutoffAngle = 30.0f;
			float outerCutoffAngle = 45.0f;

			NLOHMANN_DEFINE_TYPE_INTRUSIVE(SpotLightComponent3D, color, ambientIntensity, specularIntensity, specularExponent,
				constantAttenuation, linearAttenuation, quadraticAttenuation,
				innerCutoffAngle, outerCutoffAngle)
		};
	}

	template<>
	inline void reflection::RegisterType<rendering::SpotLightComponent3D>()
	{
		using namespace rendering;

		entt::meta<SpotLightComponent3D>()
			.type(entt::hs("SpotLightComponent3D"))
			.data<&SpotLightComponent3D::color>(entt::hs("color"))
			.data<&SpotLightComponent3D::ambientIntensity>(entt::hs("ambientIntensity"))
			.data<&SpotLightComponent3D::specularIntensity>(entt::hs("specularIntensity"))
			.data<&SpotLightComponent3D::specularExponent>(entt::hs("specularExponent"))
			.data<&SpotLightComponent3D::constantAttenuation>(entt::hs("constantAttenuation"))
			.data<&SpotLightComponent3D::linearAttenuation>(entt::hs("linearAttenuation"))
			.data<&SpotLightComponent3D::quadraticAttenuation>(entt::hs("quadraticAttenuation"))
			.data<&SpotLightComponent3D::innerCutoffAngle>(entt::hs("innerCutoffAngle"))
			.data<&SpotLightComponent3D::outerCutoffAngle>(entt::hs("outerCutoffAngle"));
	}

	namespace serialization
	{
		template<>
		inline void Serialize<rendering::SpotLightComponent3D>(const rendering::SpotLightComponent3D& data, Archive& archive)
		{
			archive.Set("color", data.color);
			archive.Set("ambientIntensity", data.ambientIntensity);
			archive.Set("specularIntensity", data.specularIntensity);
			archive.Set("specularExponent", data.specularExponent);
			archive.Set("constantAttenuation", data.constantAttenuation);
			archive.Set("linearAttenuation", data.linearAttenuation);
			archive.Set("quadraticAttenuation", data.quadraticAttenuation);
			archive.Set("innerCutoffAngle", data.innerCutoffAngle);
			archive.Set("outerCutoffAngle", data.outerCutoffAngle);
		}

		template<>
		inline void Deserialize<rendering::SpotLightComponent3D>(const Archive& archive, rendering::SpotLightComponent3D& data)
		{
			archive.Get("color", data.color);
			archive.Get("ambientIntensity", data.ambientIntensity);
			archive.Get("specularIntensity", data.specularIntensity);
			archive.Get("specularExponent", data.specularExponent);
			archive.Get("constantAttenuation", data.constantAttenuation);
			archive.Get("linearAttenuation", data.linearAttenuation);
			archive.Get("quadraticAttenuation", data.quadraticAttenuation);
			archive.Get("innerCutoffAngle", data.innerCutoffAngle);
			archive.Get("outerCutoffAngle", data.outerCutoffAngle);
		}
	}
}
