#pragma once

//#include "BaseComponent.h"
//#include "EntityTransform.h"

#include "Vector.h"

namespace Puffin
{
	/*class TransformComponent : public BaseComponent
	{
	public:

		TransformComponent(EntityTransform transform);
		TransformComponent(Vector3 position_ = Vector3(0.0f, 0.0f, 0.0f), Vector3 rotation_ = Vector3(0.0f, 0.0f, 0.0f), Vector3 scale_ = Vector3(1.0f, 1.0f, 1.0f));
		virtual ~TransformComponent();

		inline void SetTransform(EntityTransform transform_) { transform = transform_; };
		inline void SetTransform(Vector3 position_, Vector3 rotation_, Vector3 scale_)
		{
			transform.position = position_;
			transform.rotation = rotation_;
			transform.scale = scale_;
		};

		inline void SetPosition(Vector3 position_) { transform.position = position_; };
		inline void SetPosition(float x, float y, float z)
		{
			transform.position.x = x;
			transform.position.y = y;
			transform.position.z = z;
		};

		inline void SetRotation(Vector3 rotation_) { transform.rotation = rotation_; };
		inline void SetRotation(float x, float y, float z)
		{
			transform.rotation.x = x;
			transform.rotation.y = y;
			transform.rotation.z = z;
		};

		inline void SetScale(Vector3 scale_) { transform.scale = scale_; };
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
	};*/

	struct TransformComponent
	{
		Puffin::Vector3 position;
		Puffin::Vector3 rotation;
		Puffin::Vector3 scale;
	};
}
