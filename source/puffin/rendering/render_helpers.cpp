#include "puffin/rendering/render_helpers.h"

namespace puffin::rendering::util
{
	// Vector of points of each corner for ndc clip space frustum
	const std::vector<glm::vec4> g_cube_ndc =
	{
		glm::vec4(-1.0f, -1.0f, -1.0f, 1.0f),
		glm::vec4(1.0f, -1.0f, -1.0f, 1.0f),
		glm::vec4(1.0f, -1.0f, 1.0f, 1.0f),
		glm::vec4(-1.0f, -1.0f, 1.0f, 1.0f),
		glm::vec4(-1.0f, 1.0f, -1.0f, 1.0f),
		glm::vec4(1.0f, 1.0f, -1.0f, 1.0f),
		glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
		glm::vec4(-1.0f, 1.0f, 1.0f, 1.0f)
	};

	
	void calculate_camera_frustum(const glm::mat4& cam_view_proj_inv, std::vector<glm::vec4> camera_frustum_vertices_out)
	{
		camera_frustum_vertices_out.reserve(g_cube_ndc.size());

		for (const auto& vertex_ndc : g_cube_ndc)
		{
			glm::vec4 vertex_world = cam_view_proj_inv * vertex_ndc;
			vertex_world /= vertex_world.w;
			camera_frustum_vertices_out.push_back(vertex_world);
		}
	}

	glm::mat4 calculate_ortho_projection_around_camera_frustum(const std::vector<glm::vec4>& camera_frustum_vertices)
	{
		return glm::identity<glm::mat4>();
	}
}
