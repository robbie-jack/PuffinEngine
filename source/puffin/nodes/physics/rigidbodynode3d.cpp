#include "puffin/nodes/physics/rigidbodynode3d.h"

#include "puffin/components/physics/3d/rigidbodycomponent3d.h"

namespace puffin::physics
{
	RigidbodyNode3D::RigidbodyNode3D(const std::shared_ptr<core::Engine>& engine, const UUID& id)
		: TransformNode3D(engine, id)
	{
		mName = "Rigidbody3D";

		AddComponent<RigidbodyComponent3D>();
	}
}
