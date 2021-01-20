#pragma once

#ifndef TRANSFORM_COMPONENT_H
#define TRANSFORM_COMPONENT_H

#include <Types/Vector.h>
#include <Components/BaseComponent.h>

#include <cereal/cereal.hpp>

namespace Puffin
{
	struct TransformComponent : public BaseComponent
	{
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
