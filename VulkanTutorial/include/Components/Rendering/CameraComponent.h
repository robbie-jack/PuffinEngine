#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Types/Vector.h"

#include "nlohmann/json.hpp"

#define PFN_USE_DOUBLE_PRECISION

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
			float zFar = 10000.0f;
			float aspect = 0.0f;
			float fov = 60.0f;
			float prevFov = 0.0f;

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

		struct EditorCamera : public CameraComponent
		{
#ifdef PFN_USE_DOUBLE_PRECISION
			Vector3d position;
#else
			Vector3f position;
#endif
		};

		static void UpdateCameraPerspective(CameraComponent& camera, float fov, float aspect, float zNear, float zFar)
		{
			camera.prevFov = camera.fov;
			camera.fov = fov;
			camera.aspect = aspect;
			camera.zNear = zNear;
			camera.zFar = zFar;

			camera.matrices.perspective = glm::perspective(glm::radians(camera.fov), camera.aspect, camera.zNear, camera.zFar);
		}

		static void UpdateCameraFov(CameraComponent& camera, float fov_)
		{
			camera.fov = fov_;

			camera.matrices.perspective = glm::perspective(glm::radians(camera.fov), camera.aspect, camera.zNear, camera.zFar);
		}

		static glm::mat4 UpdateCameraView(const Vector3f& position, const Vector3f& lookat, const Vector3f& up)
		{
			return glm::lookAt(static_cast<glm::vec3>(position), static_cast<glm::vec3>(lookat), static_cast<glm::vec3>(up));
		}
	}
}