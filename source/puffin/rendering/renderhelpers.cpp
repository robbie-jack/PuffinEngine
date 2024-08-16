#include "puffin/rendering/renderhelpers.h"

#include "puffin/types/aabb.h"
#include "puffin/types/quat.h"

namespace puffin::rendering::util
{
	// Vector of points of each corner for ndc clip space frustum
	const std::vector<glm::vec4> gCubeNDC =
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

	void CalculateCameraFrustum(std::vector<glm::vec4>& cameraFrustumVerticesOut, const glm::mat4& camView, const glm::mat4& camProj)
	{
		const glm::mat4 camViewProjInv = glm::inverse(camProj * camView);

		cameraFrustumVerticesOut.reserve(gCubeNDC.size());

		for (const auto& vertexNdc : gCubeNDC)
		{
			glm::vec4 vertexWorld = camViewProjInv * vertexNdc;
			vertexWorld /= vertexWorld.w;
			cameraFrustumVerticesOut.push_back(vertexWorld);
		}
	}

	AABB3D CalculateAABBFromVertices(const std::vector<glm::vec4>& vertices, const float& boundsMult)
	{
		AABB3D aabb;

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
			aabb.min.x *= boundsMult;
		}
		else
		{
			aabb.min.x /= boundsMult;
		}

		// Multiple max x by bounds
		if (aabb.max.x > 0)
		{
			aabb.max.x *= boundsMult;
		}
		else
		{
			aabb.max.x /= boundsMult;
		}

		// Multiple min y by bounds
		if (aabb.min.y < 0)
		{
			aabb.min.y *= boundsMult;
		}
		else
		{
			aabb.min.y /= boundsMult;
		}

		// Multiple max y by bounds
		if (aabb.max.y > 0)
		{
			aabb.max.y *= boundsMult;
		}
		else
		{
			aabb.max.y /= boundsMult;
		}

		// Multiple min z by bounds
		if (aabb.min.z < 0)
		{
			aabb.min.z *= boundsMult;
		}
		else
		{
			aabb.min.z /= boundsMult;
		}

		// Multiple max z by bounds
		if (aabb.max.z > 0)
		{
			aabb.max.z *= boundsMult;
		}
		else
		{
			aabb.max.z /= boundsMult;
		}

		return aabb;
	}

	bool CheckAABBIsEnclosed(const AABB3D& innerAABB, const AABB3D& outerAABB)
	{
		return outerAABB.min.x < innerAABB.min.x && outerAABB.max.x > innerAABB.max.x &&
			outerAABB.min.y < innerAABB.min.y && outerAABB.max.y > innerAABB.max.y &&
			outerAABB.min.z < innerAABB.min.z && outerAABB.max.z > innerAABB.max.z;
	}
}
