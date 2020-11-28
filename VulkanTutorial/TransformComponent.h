#pragma once

#include "Vector.h"
#include "BaseComponent.h"

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
