#include "puffin/nodes/physics/rigidbodynode3d.h"

#include "puffin/components/physics/3d/rigidbodycomponent3d.h"

namespace puffin::physics
{
	void RigidbodyNode3D::Initialize()
	{
		TransformNode3D::Initialize();

		AddComponent<RigidbodyComponent3D>();
	}

	void RigidbodyNode3D::Deinitialize()
	{
		TransformNode3D::Deinitialize();

		RemoveComponent<RigidbodyComponent3D>();
	}
}
