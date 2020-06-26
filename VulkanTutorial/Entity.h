#pragma once

#include "BaseComponent.h"
#include "MeshComponent.h"
#include "ReactPhysicsComponent.h"
#include "TransformComponent.h"

#include <vector>

namespace Puffin
{
	class Entity
	{
	public:

		Entity(uint32_t id);

		void AttachComponent(BaseComponent* component);

		template <class T>
		void DetachComponent();

		inline uint32_t GetID() { return entityID; };

		template<typename T>
		T* GetComponent();

		Rendering::MeshComponent* GetMeshComponent();
		Physics::ReactPhysicsComponent* GetPhysicsComponent();
		TransformComponent* GetTransformComponent();

	protected:
		uint32_t entityID;

		// Array of handles to components that are attached to enitity
		std::vector<BaseComponent*> entityComponents;
	};
}