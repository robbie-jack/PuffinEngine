#include "ReactPhysicsComponent.h"

ReactPhysicsComponent::ReactPhysicsComponent()
{
	type = ComponentType::PHYSICS;
	prevTransform = Transform::identity();
}

ReactPhysicsComponent::~ReactPhysicsComponent()
{
	body = NULL;
	prevTransform = Transform::identity();
}