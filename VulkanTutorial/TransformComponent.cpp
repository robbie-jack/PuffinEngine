#include "TransformComponent.h"

TransformComponent::TransformComponent(glm::vec3 position_, glm::vec3 rotation_, glm::vec3 scale_)
{
	type = ComponentType::TRANSFORM;
	position = position_;
	rotation = rotation_;
	scale = scale_;
}

TransformComponent::~TransformComponent()
{

}