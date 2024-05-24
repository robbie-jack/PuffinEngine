#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "puffin/types/vector.h"

#include "nlohmann/json.hpp"

namespace puffin
{
	namespace rendering
	{
		struct CameraComponent
		{
			CameraComponent() {}

			float z_near = 0.01f;
			float z_far = 10000.0f;
			float aspect = 0.0f;
			float fov_y = 60.0f;
			float prev_fov_y = 60.0f;

			Vector3f look_at = Vector3f(0.0f);
			Vector3f up = Vector3f(0.0f, 1.0f, 0.0f);
			Vector3f right = Vector3f(1.0f, 0.0f, 0.0f);

			glm::mat4 view = glm::mat4(0.0f);
			glm::mat4 proj = glm::mat4(0.0f);
			glm::mat4 view_proj = glm::mat4(0.0f);

			NLOHMANN_DEFINE_TYPE_INTRUSIVE(CameraComponent, z_near, z_far, aspect, fov_y, up)
		};

		struct EditorCamera : public CameraComponent
		{
#ifdef PFN_USE_DOUBLE_PRECISION
			Vector3d position;
#else
			Vector3f position;
#endif

			Vector3f direction = Vector3f(0.0f, 0.0f, -1.0f);

			float yaw = -90.0f;
			float pitch = 0.0f;
			float speed = 25.0f;
		};
	}
}