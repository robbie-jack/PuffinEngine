#pragma once

#include <vector>

#include "puffin/components/rendering/camera_component.h"
#include "puffin/components/transform_component_3d.h"
#include "puffin/types/Quat.h"
#include "glm/vec4.hpp"

namespace puffin::rendering::util
{
	// Calculate camera frustum vertices in world space
	void calculate_camera_frustum(std::vector<glm::vec4>& camera_frustum_vertices_out, const glm::mat4& cam_view, const glm::mat4& cam_proj, float bounds_multiplier = 1.0);

	// Calculate ortho projection matrix around a camera view frustum (world space)
	glm::mat4 calculate_ortho_projection_around_camera_frustum(const std::vector<glm::vec4>& camera_frustum_vertices, const glm::mat4& light_view, const float z_mult = 10.0f);
}
