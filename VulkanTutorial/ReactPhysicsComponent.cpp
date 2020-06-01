#include "ReactPhysicsComponent.h"

ReactPhysicsComponent::ReactPhysicsComponent()
{
	type = ComponentType::PHYSICS;
	prevTransform = rp3d::Transform::identity();
}

ReactPhysicsComponent::~ReactPhysicsComponent()
{
	body = NULL;
	prevTransform = rp3d::Transform::identity();
}