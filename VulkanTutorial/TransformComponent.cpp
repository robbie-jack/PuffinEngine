#include "TransformComponent.h"

namespace Puffin
{
	TransformComponent::TransformComponent(EntityTransform transform_)
	{
		type = ComponentType::TRANSFORM;
		name = "Transform Component";
		transform = transform_;
	}

	TransformComponent::TransformComponent(Vector3 position_, Vector3 rotation_, Vector3 scale_)
	{
		type = ComponentType::TRANSFORM;
		name = "Transform Component";
		transform.position = position_;
		transform.rotation = rotation_;
		transform.scale = scale_;
	}

	TransformComponent::~TransformComponent()
	{

	}
}