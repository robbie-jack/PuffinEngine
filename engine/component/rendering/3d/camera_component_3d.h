#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include "nlohmann/json.hpp"

#include "types/vector3.h"
#include "utility/reflection.h"
#include "utility/serialization.h"

namespace puffin
{
	namespace rendering
	{
		struct CameraComponent3D
		{
			CameraComponent3D() = default;

			bool active = false;

			float zNear = 0.01f;
			float zFar = 10000.0f;
			float aspect = 0.0f;
			float fovY = 60.0f;
			float prevFovY = 60.0f;

			Vector3f direction = { 0.0f, 0.0f, -1.0f };
			Vector3f up = Vector3f(0.0f, 1.0f, 0.0f);
			Vector3f right = Vector3f(1.0f, 0.0f, 0.0f);

			glm::mat4 view = glm::mat4(0.0f);
			glm::mat4 proj = glm::mat4(0.0f);
			glm::mat4 viewProj = glm::mat4(0.0f);

			NLOHMANN_DEFINE_TYPE_INTRUSIVE(CameraComponent3D, zNear, zFar, aspect, fovY, up)
		};
	}

	template<>
	inline void reflection::RegisterType<rendering::CameraComponent3D>()
	{
		using namespace rendering;

		entt::meta<CameraComponent3D>()
			.type(entt::hs("CameraComponent3D"))
			.data<&CameraComponent3D::active>(entt::hs("active"))
			.data<&CameraComponent3D::zNear>(entt::hs("zNear"))
			.data<&CameraComponent3D::zFar>(entt::hs("zFar"))
			.data<&CameraComponent3D::aspect>(entt::hs("aspect"))
			.data<&CameraComponent3D::fovY>(entt::hs("fovY"))
			.data<&CameraComponent3D::direction>(entt::hs("direction"))
			.data<&CameraComponent3D::up>(entt::hs("up"))
			.data<&CameraComponent3D::right>(entt::hs("right"));
	}

	namespace serialization
	{
		template<>
		inline nlohmann::json Serialize<rendering::CameraComponent3D>(const rendering::CameraComponent3D& data)
		{
			nlohmann::json json;

			json["active"] = data.active;
			json["zNear"] = data.zNear;
			json["zFar"] = data.zFar;
			json["aspect"] = data.aspect;
			json["fovY"] = data.fovY;
			json["up"] = Serialize(data.up);
			
			return json;
		}

		template<>
		inline rendering::CameraComponent3D Deserialize<rendering::CameraComponent3D>(const nlohmann::json& json)
		{
			rendering::CameraComponent3D data;

			data.active = json["active"];
			data.zNear = json["zNear"];
			data.zFar = json["zFar"];
			data.aspect = json["aspect"];
			data.fovY = json["fovY"];
			data.up = Deserialize<Vector3f>(json["up"]);
			
			return data;
		}
	}
}