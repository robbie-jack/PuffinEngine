#pragma once

#ifndef TRANSFORM_COMPONENT_H
#define TRANSFORM_COMPONENT_H

#include <Types/Vector.h>

#include <cereal/cereal.hpp>

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

		T position = T(0.0f);
		Vector3f rotation = Vector3f(0.0f);
		Vector3f scale = Vector3f(1.0f);
	};

	#ifdef PFN_USE_DOUBLE_PRECISION
		typedef Transform<Vector3d> TransformComponent;
	#else
		typedef Transform<Vector3f> TransformComponent;
	#endif

	template<class Archive>
	void save(Archive& archive, const TransformComponent& comp)
	{
		Vector3d position = comp.position;
			
		archive(CEREAL_NVP(position), CEREAL_NVP(comp.rotation), CEREAL_NVP(comp.scale));
	}

	template<class Archive>
	void load(Archive& archive, TransformComponent& comp)
	{
		Vector3d position;

		archive(CEREAL_NVP(position), CEREAL_NVP(comp.rotation), CEREAL_NVP(comp.scale));

		comp.position = position;
	}
}

#endif // TRANSFORM_COMPONENT_H
