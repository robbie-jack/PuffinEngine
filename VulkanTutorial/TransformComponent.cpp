#include "TransformComponent.h"

using namespace Puffin;

TransformComponent::TransformComponent(EntityTransform transform_)
{
	type = ComponentType::TRANSFORM;
	transform = transform_;
}

TransformComponent::TransformComponent(glm::vec3 position_, glm::vec3 rotation_, glm::vec3 scale_)
{
	type = ComponentType::TRANSFORM;
	transform.position = position_;
	transform.rotation = rotation_;
	transform.scale = scale_;
}

TransformComponent::~TransformComponent()
{

}