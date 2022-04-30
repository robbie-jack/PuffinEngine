#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Types/Vector.h"

#include "nlohmann/json.hpp"

namespace Puffin
{
	namespace Rendering
	{
		struct CameraMatrices
		{
			glm::mat4 perspective;
			glm::mat4 view;
		};

		struct ViewData
		{
			alignas(16) glm::vec3 viewPos;
		};

		struct CameraComponent
		{
			CameraComponent() {}

			float zNear = 0.01f;
			float zFar = 100.0f;
			float aspect = 16.0f / 9.0f;
			float fov = 60.0f;
			float prevFov = 90.0f;

			Vector3f position;
			Vector3f lookat;

			Vector3f direction = glm::vec3(0.0f, 0.0f, -1.0f);
			Vector3f up = glm::vec3(0.0f, 1.0f, 0.0f);
			Vector3f right;

			float yaw = -90.0f;
			float pitch = 0.0f;
			float speed = 5.0f;

			CameraMatrices matrices;

			ViewData data;

			NLOHMANN_DEFINE_TYPE_INTRUSIVE(CameraComponent, zNear, zFar, aspect, fov,
				lookat, direction, up, right,
				yaw, pitch, speed)
		};

		static void UpdatePerspective(CameraComponent& camera, float fov_, float aspect_, float zNear_, float zFar_)
		{
			camera.fov = fov_;
			camera.aspect = aspect_;
			camera.zNear = zNear_;
			camera.zFar = zFar_;

			camera.matrices.perspective = glm::perspective(glm::radians(camera.fov), camera.aspect, camera.zNear, camera.zFar);
		}

		static void UpdateFov(CameraComponent& camera, float fov_)
		{
			camera.fov = fov_;

			camera.matrices.perspective = glm::perspective(glm::radians(camera.fov), camera.aspect, camera.zNear, camera.zFar);
		}

		static void UpdateViewMatrix(CameraComponent& camera)
		{
			camera.matrices.view = glm::lookAt(static_cast<glm::vec3>(camera.position), static_cast<glm::vec3>(camera.position + camera.direction), static_cast<glm::vec3>(camera.up));
		}
	}
}