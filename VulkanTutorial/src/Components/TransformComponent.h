#pragma once

#ifndef TRANSFORM_COMPONENT_H
#define TRANSFORM_COMPONENT_H

#include <Components/BaseComponent.h>

#include <Types/Vector.h>

#include <cereal/cereal.hpp>

namespace Puffin
{
	struct TransformComponent : public BaseComponent
	{
		TransformComponent()
		{
			bFlagCreated = false;
			bFlagDeleted = false;
		}

		TransformComponent(Puffin::Vector3 InPosition, Puffin::Vector3 InRotation, Puffin::Vector3 InScale) :
			position(InPosition), rotation(InRotation), scale(InScale)
		{
			TransformComponent();
		}

		Puffin::Vector3 position;
		Puffin::Vector3 rotation;
		Puffin::Vector3 scale;
	};

	template<class Archive>
	void serialize(Archive& archive, TransformComponent& comp)
	{
		archive(CEREAL_NVP(comp.position), CEREAL_NVP(comp.rotation), CEREAL_NVP(comp.scale));
	}
}

#endif // TRANSFORM_COMPONENT_H
