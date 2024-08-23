#pragma once

#include "nlohmann/json.hpp"

#include "puffin/components/rendering/3d/lightcomponent3d.h"
#include "puffin/utility/reflection.h"
#include "puffin/utility/serialization.h"

namespace puffin
{
	namespace rendering
	{
		/*
		 * Component containing variables used by directional lights
		 */
		struct DirectionalLightComponent3D : LightComponent3D
		{
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

	namespace serialization
	{
		template<>
		inline void Serialize<rendering::DirectionalLightComponent3D>(const rendering::DirectionalLightComponent3D& data, Archive& archive)
		{
			archive.Set("color", data.color);
			archive.Set("ambientIntensity", data.ambientIntensity);
			archive.Set("specularIntensity", data.specularIntensity);
			archive.Set("specularExponent", data.specularExponent);
		}

		template<>
		inline void Deserialize<rendering::DirectionalLightComponent3D>(const Archive& archive, rendering::DirectionalLightComponent3D& data)
		{
			archive.Get("color", data.color);
			archive.Get("ambientIntensity", data.ambientIntensity);
			archive.Get("specularIntensity", data.specularIntensity);
			archive.Get("specularExponent", data.specularExponent);
		}
	}
}
