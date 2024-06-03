#include "puffin/rendering/render_helpers.h"

#include "puffin/types/aabb.h"
#include "puffin/types/Quat.h"
#include "puffin/types/Quat.h"
#include "puffin/types/Quat.h"
#include "puffin/types/Quat.h"
#include "puffin/types/Quat.h"
#include "puffin/types/Quat.h"
#include "puffin/types/Quat.h"
#include "puffin/types/Quat.h"
#include "puffin/types/Quat.h"
#include "puffin/types/Quat.h"
#include "puffin/types/Quat.h"
#include "puffin/types/Quat.h"
#include "puffin/types/Quat.h"
#include "puffin/types/Quat.h"
#include "puffin/types/Quat.h"
#include "puffin/types/Quat.h"
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

	void calculate_camera_frustum(std::vector<glm::vec4>& camera_frustum_vertices_out, const glm::mat4& cam_view, const glm::mat4& cam_proj)
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

	AABB_3D calculate_aabb_from_vertices(const std::vector<glm::vec4>& vertices, const float& bounds_mult)
	{
		AABB_3D aabb;

		aabb.min.x = std::numeric_limits<float>::max();
		aabb.max.x = std::numeric_limits<float>::lowest();
		aabb.min.y = std::numeric_limits<float>::max();
		aabb.max.y = std::numeric_limits<float>::lowest();
		aabb.min.z = std::numeric_limits<float>::max();
		aabb.max.z = std::numeric_limits<float>::lowest();

		for (const auto& v : vertices)
		{
			aabb.min.x = std::min(aabb.min.x, v.x);
			aabb.max.x = std::max(aabb.max.x, v.x);
			aabb.min.y = std::min(aabb.min.y, v.y);
			aabb.max.y = std::max(aabb.max.y, v.y);
			aabb.min.z = std::min(aabb.min.z, v.z);
			aabb.max.z = std::max(aabb.max.z, v.z);
		}

		// Multiple min x by bounds
		if (aabb.min.x < 0)
		{
			aabb.min.x *= bounds_mult;
		}
		else
		{
			aabb.min.x /= bounds_mult;
		}

		// Multiple max x by bounds
		if (aabb.max.x > 0)
		{
			aabb.max.x *= bounds_mult;
		}
		else
		{
			aabb.max.x /= bounds_mult;
		}

		// Multiple min y by bounds
		if (aabb.min.y < 0)
		{
			aabb.min.y *= bounds_mult;
		}
		else
		{
			aabb.min.y /= bounds_mult;
		}

		// Multiple max y by bounds
		if (aabb.max.y > 0)
		{
			aabb.max.y *= bounds_mult;
		}
		else
		{
			aabb.max.y /= bounds_mult;
		}

		// Multiple min z by bounds
		if (aabb.min.z < 0)
		{
			aabb.min.z *= bounds_mult;
		}
		else
		{
			aabb.min.z /= bounds_mult;
		}

		// Multiple max z by bounds
		if (aabb.max.z > 0)
		{
			aabb.max.z *= bounds_mult;
		}
		else
		{
			aabb.max.z /= bounds_mult;
		}

		return aabb;
	}

	bool check_aabb_is_enclosed(const AABB_3D& inner_aabb, const AABB_3D& outer_aabb)
	{
		return outer_aabb.min.x < inner_aabb.min.x && outer_aabb.max.x > inner_aabb.max.x &&
			outer_aabb.min.y < inner_aabb.min.y && outer_aabb.max.y > inner_aabb.max.y &&
			outer_aabb.min.z < inner_aabb.min.z && outer_aabb.max.z > inner_aabb.max.z;
	}
}
