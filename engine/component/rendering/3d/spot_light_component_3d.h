#pragma once

#include "nlohmann/json.hpp"

#include "component/rendering/3d/point_light_component_3d.h"
#include "utility/reflection.h"
#include "utility/serialization.h"

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
		inline nlohmann::json Serialize<rendering::SpotLightComponent3D>(const rendering::SpotLightComponent3D& data)
		{
			nlohmann::json json;

			json["color"] = Serialize(data.color);
			json["ambientIntensity"] = data.ambientIntensity;
			json["specularIntensity"] = data.specularIntensity;
			json["specularExponent"] = data.specularExponent;
			json["constantAttenuation"] = data.constantAttenuation;
			json["linearAttenuation"] = data.linearAttenuation;
			json["quadraticAttenuation"] = data.quadraticAttenuation;
			json["innerCutoffAngle"] = data.innerCutoffAngle;
			json["outerCutoffAngle"] = data.outerCutoffAngle;

			return json;
		}

		template<>
		inline rendering::SpotLightComponent3D Deserialize<rendering::SpotLightComponent3D>(const nlohmann::json& json)
		{
			rendering::SpotLightComponent3D data;

			data.color = Deserialize<Vector3f>(json["color"]);
			data.ambientIntensity = json["ambientIntensity"];
			data.specularIntensity = json["specularIntensity"];
			data.specularExponent = json["specularExponent"];
			data.constantAttenuation = json["constantAttenuation"];
			data.linearAttenuation = json["linearAttenuation"];
			data.quadraticAttenuation = json["quadraticAttenuation"];
			data.innerCutoffAngle = json["innerCutoffAngle"];
			data.outerCutoffAngle = json["outerCutoffAngle"];
			
			return data;
		}
	}
}
