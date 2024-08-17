#pragma once

#include "puffin/nodes/transformnode3d.h"

namespace puffin::physics
{
	class RigidbodyNode3D : public TransformNode3D
	{
	public:

		explicit RigidbodyNode3D(const std::shared_ptr<core::Engine>& engine, const UUID& id = gInvalidID);
		~RigidbodyNode3D() override = default;

	};
}