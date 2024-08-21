#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include "nlohmann/json.hpp"

#include "puffin/types/vector3.h"
#include "puffin/utility/reflection.h"
#include "puffin/utility/serialization.h"

namespace puffin
{
	namespace rendering
	{
		struct CameraComponent3D
		{
			CameraComponent3D() = default;

			bool active = false;

			float zNear = 0.01f;
			float zFar = 200.0f;
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
			.data<&CameraComponent3D::right>(entt::hs("right"))
			.data<&CameraComponent3D::view>(entt::hs("view"))
			.data<&CameraComponent3D::proj>(entt::hs("proj"))
			.data<&CameraComponent3D::viewProj>(entt::hs("viewProj"));
	}

	namespace serialization
	{
		template<>
		inline void Serialize<rendering::CameraComponent3D>(const rendering::CameraComponent3D& type, Archive& archive)
		{
			archive.Set("active", type.active);
			archive.Set("zNear", type.zNear);
			archive.Set("zFar", type.zFar);
			archive.Set("aspect", type.aspect);
			archive.Set("fovY", type.fovY);
			archive.Set("up", type.up);
		}

		template<>
		inline void Deserialize<rendering::CameraComponent3D>(const Archive& archive, rendering::CameraComponent3D& type)
		{
			archive.Get("active", type.active);
			archive.Get("zNear", type.zNear);
			archive.Get("zFar", type.zFar);
			archive.Get("aspect", type.aspect);
			archive.Get("fovY", type.fovY);
			archive.DeserializeComplexType("up", type.up);
		}
	}
}