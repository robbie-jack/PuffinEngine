#pragma once

#include <vector>

#include "glm/vec4.hpp"

#include "puffin/types/aabb.h"
#include "puffin/types/quat.h"

namespace puffin::rendering::util
{
	// Calculate camera frustum vertices in world space
	void CalculateCameraFrustum(std::vector<glm::vec4>& cameraFrustumVerticesOut, const glm::mat4& camView, const glm::mat4& camProj);

	AABB3D CalculateAABBFromVertices(const std::vector<glm::vec4>& vertices, const float& boundsMult = 1.0f);

	bool CheckAABBIsEnclosed(const AABB3D& innerAABB, const AABB3D& outerAABB);
}
