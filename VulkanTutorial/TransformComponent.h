#pragma once

#include "BaseComponent.h"
#include "EntityTransform.h"

namespace Puffin
{
	class TransformComponent : public BaseComponent
	{
	public:

		TransformComponent(EntityTransform transform);
		TransformComponent(glm::vec3 position_ = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 rotation_ = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 scale_ = glm::vec3(1.0f, 1.0f, 1.0f));
		virtual ~TransformComponent();

		inline void SetTransform(EntityTransform transform_) { transform = transform_; };
		inline void SetTransform(glm::vec3 position_, glm::vec3 rotation_, glm::vec3 scale_)
		{
			transform.position = position_;
			transform.rotation = rotation_;
			transform.scale = scale_;
		};

		inline void SetPosition(glm::vec3 position_) { transform.position = position_; };
		inline void SetPosition(float x, float y, float z)
		{
			transform.position.x = x;
			transform.position.y = y;
			transform.position.z = z;
		};

		inline void SetRotation(glm::vec3 rotation_) { transform.rotation = rotation_; };
		inline void SetRotation(float x, float y, float z)
		{
			transform.rotation.x = x;
			transform.rotation.y = y;
			transform.rotation.z = z;
		};

		inline void SetScale(glm::vec3 scale_) { transform.scale = scale_; };
		inline void SetScale(float x, float y, float z)
		{
			transform.scale.x = x;
			transform.scale.y = y;
			transform.scale.z = z;
		};

		inline EntityTransform GetTransform() { return transform; };
		inline Puffin::Vector3 GetPosition() { return transform.position; };
		inline Puffin::Vector3 GetRotation() { return transform.rotation; };
		inline Puffin::Vector3 GetScale() { return transform.scale; };

	protected:

		EntityTransform transform;
	};
}
