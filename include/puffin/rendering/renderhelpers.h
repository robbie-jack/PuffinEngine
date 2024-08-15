#pragma once

#include <vector>

#include "glm/vec4.hpp"

#include "puffin/components/rendering/cameracomponent.h"
#include "puffin/components/transformcomponent3d.h"
#include "puffin/types/aabb.h"
#include "puffin/types/quat.h"
#include "puffin/types/quat.h"
#include "puffin/types/quat.h"

namespace puffin::rendering::util
{
	// Calculate camera frustum vertices in world space
	void calculate_camera_frustum(std::vector<glm::vec4>& camera_frustum_vertices_out, const glm::mat4& cam_view, const glm::mat4& cam_proj);

	AABB_3D calculate_aabb_from_vertices(const std::vector<glm::vec4>& vertices, const float& bounds_mult = 1.0f);

	bool check_aabb_is_enclosed(const AABB_3D& inner_aabb, const AABB_3D& outer_aabb);
}
