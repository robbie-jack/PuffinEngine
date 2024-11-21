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
		inline nlohmann::json Serialize<rendering::DirectionalLightComponent3D>(const rendering::DirectionalLightComponent3D& data)
		{
			nlohmann::json json;

			json["color"] = Serialize(data.color);
			json["ambientIntensity"] = data.ambientIntensity;
			json["specularIntensity"] = data.specularIntensity;
			json["specularExponent"] = data.specularExponent;

			return json;
		}

		template<>
		inline rendering::DirectionalLightComponent3D Deserialize<rendering::DirectionalLightComponent3D>(const nlohmann::json& json)
		{
			rendering::DirectionalLightComponent3D data;

			data.color = Deserialize<Vector3f>(json["color"]);
			data.ambientIntensity = json["ambientIntensity"];
			data.specularIntensity = json["specularIntensity"];
			data.specularExponent = json["specularExponent"];
			
			return data;
		}
	}
}
