#include "TransformComponent.h"

TransformComponent::TransformComponent(Transform transform_)
{
	type = ComponentType::TRANSFORM;
	transform = transform_;
}

TransformComponent::TransformComponent(glm::vec3 position, glm::vec3 rotation, glm::vec3 scale)
{
	type = ComponentType::TRANSFORM;
	transform.position = position;
	transform.rotation = rotation;
	transform.scale = scale;
}

TransformComponent::~TransformComponent()
{

}