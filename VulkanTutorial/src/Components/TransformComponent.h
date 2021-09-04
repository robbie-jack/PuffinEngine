#pragma once

#ifndef TRANSFORM_COMPONENT_H
#define TRANSFORM_COMPONENT_H

#include <Types/Vector.h>

#include <cereal/cereal.hpp>

namespace Puffin
{
	struct TransformComponent
	{
		TransformComponent() {}

		TransformComponent(Puffin::Vector3 InPosition, Puffin::Vector3 InRotation, Puffin::Vector3 InScale) :
			position(InPosition), rotation(InRotation), scale(InScale)
		{
		}

		Puffin::Vector3 position = Puffin::Vector3(0.0f);
		Puffin::Vector3 rotation = Puffin::Vector3(0.0f);
		Puffin::Vector3 scale = Puffin::Vector3(1.0f);
	};

	template<class Archive>
	void serialize(Archive& archive, TransformComponent& comp)
	{
		archive(CEREAL_NVP(comp.position), CEREAL_NVP(comp.rotation), CEREAL_NVP(comp.scale));
	}
}

#endif // TRANSFORM_COMPONENT_H
