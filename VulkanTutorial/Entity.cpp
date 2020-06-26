#include "Entity.h"

namespace Puffin
{
	Entity::Entity(uint32_t id)
	{
		entityID = id;
	}

	template<typename T>
	T* Entity::GetComponent()
	{
		// Iterate through entity components
		for (auto comp : entityComponents)
		{
			if (dynamic_cast<T>(comp))
			{
				return (T)comp;
			}
		}
	}

	Rendering::MeshComponent* Entity::GetMeshComponent()
	{
		// Iterate through entity components
		for (auto comp : entityComponents)
		{
			// If component is of type RENDER
			if (comp->GetType() == ComponentType::MESH)
			{
				// Return type cast component
				return (Rendering::MeshComponent*)comp;
			}
		}

		// Return nullptr if no render component is found
		return nullptr;
	}

	Physics::ReactPhysicsComponent* Entity::GetPhysicsComponent()
	{
		for (auto comp : entityComponents)
		{
			// If component is of type PHYSICS
			if (comp->GetType() == ComponentType::PHYSICS)
			{
				// Return type cast component
				return (Physics::ReactPhysicsComponent*)comp;
			}
		}

		// Return nullptr if no physics component is found
		return nullptr;
	}

	TransformComponent* Entity::GetTransformComponent()
	{
		for (auto comp : entityComponents)
		{
			// If component is of type PHYSICS
			if (comp->GetType() == ComponentType::TRANSFORM)
			{
				// Return type cast component
				return (TransformComponent*)comp;
			}
		}

		// Return nullptr if no transform component is found
		return nullptr;
	}

	void Entity::AttachComponent(BaseComponent* component)
	{
		component->SetEntityID(entityID);
		entityComponents.push_back(component);
	}

	template <class T>
	void Entity::DetachComponent()
	{
		for (auto comp : entityComponents)
		{
			if (dynamic_cast<T>(comp))
			{
				entityComponents.erase(comp);
			}
		}
	}
}