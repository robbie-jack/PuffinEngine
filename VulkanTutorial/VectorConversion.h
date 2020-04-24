#pragma once

#include "reactphysics3d.h"
#include <glm/gtc/matrix_transform.hpp>

using namespace reactphysics3d;

static inline glm::vec3 ConvertVectorToGLM(rp3d::Vector3 vector)
{
	glm::vec3 glm_vector;
	glm_vector.x = (float)vector.x;
	glm_vector.y = (float)vector.y;
	glm_vector.z = (float)vector.z;
	return glm_vector;
}

static inline rp3d::Vector3 ConvertVectorToRP3D(glm::vec3 vector)
{
	rp3d::Vector3 rp3d_vector;
	rp3d_vector.x = (decimal)vector.x;
	rp3d_vector.y = (decimal)vector.y;
	rp3d_vector.z = (decimal)vector.z;
	return rp3d_vector;
}

static rp3d::Vector3 ConvertToEulerAngles(Quaternion q)
{
	rp3d::Vector3 angles;

	// Roll (x-axis rotation)
	double sinr_cosp = 2 * (q.w * q.x + q.y * q.z);
	double cosr_cosp = 1 - 2 * (q.x * q.x + q.y * q.y);
	angles.x = std::atan2(sinr_cosp, cosr_cosp);

	// Pitch (y-axis rotation)
	double sinp = 2 * (q.w * q.y - q.z * q.x);
	if (std::abs(sinp) >= 1)
		angles.y = std::copysign(PI / 2, sinp); // use 90 degrees if out of range
	else
		angles.y = std::asin(sinp);

	// Yaw (z-axis rotation)
	double siny_cosp = 2 * (q.w * q.z + q.x * q.y);
	double cosy_cosp = 1 - 2 * (q.y * q.y + q.z * q.z);
	angles.z = std::atan2(siny_cosp, cosy_cosp);

	return angles;
}