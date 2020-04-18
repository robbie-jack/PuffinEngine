#pragma once

#include "BaseComponent.h"
#include <glm/gtc/matrix_transform.hpp>

class TransformComponent : public BaseComponent
{
public:

	TransformComponent(glm::vec3 position_ = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 rotation_ = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 scale_ = glm::vec3(0.0f, 0.0f, 0.0f));
	virtual ~TransformComponent();

	inline void SetTransform(glm::vec3 position_, glm::vec3 rotation_, glm::vec3 scale_)
	{
		position = position_;
		rotation = rotation_;
		scale = scale_;
	};

	inline void SetPosition(glm::vec3 position_) { position = position_; };
	inline void SetRotation(glm::vec3 rotation_) { rotation = rotation_; };
	inline void SetScale(glm::vec3 scale_) { scale = scale_; };

	inline glm::vec3 GetPosition() { return position; };
	inline glm::vec3 GetRotation() { return rotation; };
	inline glm::vec3 GetScale() { return scale; };

protected:

	glm::vec3 position;
	glm::vec3 rotation;
	glm::vec3 scale;
};
