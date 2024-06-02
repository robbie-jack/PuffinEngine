#pragma once

#include "puffin/types/vector.h"
#include "puffin/components/transform_component_3d.h"
#include "puffin/components/rendering/camera_component.h"

namespace puffin::rendering::util
{
	// Calculate camera frustum vertices in world space
	inline void calculate_camera_frustum(const glm::mat4& cam_view_proj_inv, std::vector<glm::vec4> camera_frustum_vertices_out);

	// Calculate ortho projection matrix around a camera view frustum (world space)
	glm::mat4 calculate_ortho_projection_around_camera_frustum(const std::vector<glm::vec4>& camera_frustum_vertices);
}