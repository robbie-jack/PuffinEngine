#pragma once

#ifndef VECTOR_CONVERSION_H
#define VECTOR_CONVERSION_H

#include "btBulletDynamicsCommon.h"

static btVector3 ConvertToEulerAngles(btQuaternion q)
{
	btVector3 angles;
	const float PI = 3.14159;

	// Roll (x-axis rotation)
	double sinr_cosp = 2 * (q.getW() * q.getX() + q.getY() * q.getZ());
	double cosr_cosp = 1 - 2 * (q.getX() * q.getX() + q.getY() * q.getY());
	angles.setX(std::atan2(sinr_cosp, cosr_cosp));

	// Pitch (y-axis rotation)
	double sinp = 2 * (q.getW() * q.getY() - q.getZ() * q.getX());
	if (std::abs(sinp) >= 1)
		angles.setY(std::copysign(PI / 2, sinp)); // use 90 degrees if out of range
	else
		angles.setY(std::asin(sinp));

	// Yaw (z-axis rotation)
	double siny_cosp = 2 * (q.getW() * q.getZ() + q.getX() * q.getY());
	double cosy_cosp = 1 - 2 * (q.getY() * q.getY() + q.getZ() * q.getZ());
	angles.setZ(std::atan2(siny_cosp, cosy_cosp));

	return angles;
}

#endif // !VECTOR_CONVERSION_H