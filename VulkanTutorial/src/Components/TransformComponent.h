#pragma once

#ifndef TRANSFORM_COMPONENT_H
#define TRANSFORM_COMPONENT_H

#include <Types/Vector.h>

#include "nlohmann/json.hpp"

//#define PFN_USE_DOUBLE_PRECISION

namespace Puffin
{
	template<typename T>
	struct Transform
	{
		Transform() {}

		Transform(T InPosition, Vector3f InRotation, Vector3f InScale) :
			position(InPosition), rotation(InRotation), scale(InScale)
		{
		}

		~Transform() {};

		Transform<T>& operator=(const Transform<T>& rhs)
		{
			position = rhs.position;
			rotation = rhs.rotation;
			scale = rhs.scale;

			return *this;
		}

		T position = T(0.0f);
		Vector3f rotation = Vector3f(0.0f);
		Vector3f scale = Vector3f(1.0f);

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(Transform, position, rotation, scale)
	};

	#ifdef PFN_USE_DOUBLE_PRECISION
		typedef Transform<Vector3d> TransformComponent;
	#else
		typedef Transform<Vector3f> TransformComponent;
	#endif
}

#endif // TRANSFORM_COMPONENT_H
