#include "ReactPhysicsComponent.h"

namespace Puffin
{
	namespace Physics
	{
		ReactPhysicsComponent::ReactPhysicsComponent()
		{
			type = ComponentType::PHYSICS;
			name = "Physics Component";
			prevTransform = rp3d::Transform::identity();
		}

		ReactPhysicsComponent::~ReactPhysicsComponent()
		{
			body = NULL;
			prevTransform = rp3d::Transform::identity();
		}
	}
}