#pragma once

#include "BaseComponent.h"
#include "Transform.h"

class TransformComponent : public BaseComponent
{
public:

	TransformComponent(Transform transfrom_);
	TransformComponent(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 rotation = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 scale = glm::vec3(0.0f, 0.0f, 0.0f));
	virtual ~TransformComponent();

	inline Transform GetTransform() { return transform; };
	inline void SetTransform(Transform transform_) { transform = transform_; };
	inline void SetTransform(glm::vec3 position, glm::vec3 rotation, glm::vec3 scale)
	{
		transform.position = position;
		transform.rotation = rotation;
		transform.scale = scale;
	};

protected:

	Transform transform;
};
