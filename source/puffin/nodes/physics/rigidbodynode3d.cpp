#include "puffin/nodes/physics/rigidbodynode3d.h"

#include "puffin/components/physics/3d/rigidbodycomponent3d.h"
#include "puffin/components/physics/3d/velocitycomponent3d.h"

namespace puffin::physics
{
	void RigidbodyNode3D::Initialize()
	{
		TransformNode3D::Initialize();

		AddComponent<RigidbodyComponent3D>();
		AddComponent<VelocityComponent3D>();
	}

	void RigidbodyNode3D::Deinitialize()
	{
		TransformNode3D::Deinitialize();

		RemoveComponent<RigidbodyComponent3D>();
		RemoveComponent<VelocityComponent3D>();
	}

	const BodyType& RigidbodyNode3D::GetBodyType() const
	{
		return GetComponent<RigidbodyComponent3D>().bodyType;
	}

	void RigidbodyNode3D::SetBodyType(const BodyType& bodyType)
	{
		mRegistry->patch<RigidbodyComponent3D>(mEntity, [&bodyType](auto& body) { body.bodyType = bodyType; });
	}

	const float& RigidbodyNode3D::GetMass() const
	{
		return GetComponent<RigidbodyComponent3D>().mass;
	}

	void RigidbodyNode3D::SetMass(const float& mass)
	{
		mRegistry->patch<RigidbodyComponent3D>(mEntity, [&mass](auto& body) { body.mass = mass; });
	}

	const float& RigidbodyNode3D::GetElasticity() const
	{
		return GetComponent<RigidbodyComponent3D>().elasticity;
	}

	void RigidbodyNode3D::SetElasticity(const float& elasticity)
	{
		mRegistry->patch<RigidbodyComponent3D>(mEntity, [&elasticity](auto& body) { body.elasticity = elasticity; });
	}
}
