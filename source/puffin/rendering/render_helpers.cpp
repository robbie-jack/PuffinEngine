#include "puffin/rendering/render_helpers.h"

#include "puffin/types/aabb.h"
#include "puffin/types/Quat.h"

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

	void calculate_camera_frustum(const glm::mat4& cam_view, const glm::mat4& cam_proj, std::vector<glm::vec4>& camera_frustum_vertices_out)
	{
		glm::mat4 cam_view_proj_inv = glm::inverse(cam_proj * cam_view);

		camera_frustum_vertices_out.reserve(g_cube_ndc.size());

		for (const auto& vertex_ndc : g_cube_ndc)
		{
			glm::vec4 vertex_world = cam_view_proj_inv * vertex_ndc;
			vertex_world /= vertex_world.w;
			camera_frustum_vertices_out.push_back(vertex_world);
		}
	}

	glm::mat4 calculate_ortho_projection_around_camera_frustum(const std::vector<glm::vec4>& camera_frustum_vertices, const glm::mat4& light_view, const float z_mult)
	{
		AABB_3D aabb;

		aabb.min.x = std::numeric_limits<float>::max();
		aabb.max.x = std::numeric_limits<float>::lowest();
		aabb.min.y = std::numeric_limits<float>::max();
		aabb.max.y = std::numeric_limits<float>::lowest();
		aabb.min.z = std::numeric_limits<float>::max();
		aabb.max.z = std::numeric_limits<float>::lowest();

		for (const auto& v : camera_frustum_vertices)
		{
			const auto trf = light_view * v;

			aabb.min.x = std::min(aabb.min.x, trf.x);
			aabb.max.x = std::max(aabb.max.x, trf.x);
			aabb.min.y = std::min(aabb.min.y, trf.y);
			aabb.max.y = std::max(aabb.max.y, trf.y);
			aabb.min.z = std::min(aabb.min.z, trf.z);
			aabb.max.z = std::max(aabb.max.z, trf.z);
		}

		if (aabb.min.z < 0)
		{
			aabb.min.z *= z_mult;
		}
		else
		{
			aabb.min.z /= z_mult;
		}

		if (aabb.max.z > 0)
		{
			aabb.max.z *= z_mult;
		}
		else
		{
			aabb.max.z /= z_mult;
		}

		return glm::ortho(aabb.min.x, aabb.max.x, aabb.min.y, aabb.max.y, aabb.min.z, aabb.max.z);
	}
}
