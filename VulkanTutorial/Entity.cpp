#include "Entity.h"

Entity::Entity(uint32_t id)
{
	entityID = id;
}

RenderComponent* Entity::GetRenderComponent()
{
	// Iterate through entity components
	for (auto comp : entityComponents)
	{
		// IF component is of type RENDER
		if (comp->GetType() == ComponentType::RENDER)
		{
			// Return type cast component
			return (RenderComponent*)comp;
		}
	}
}

void Entity::AttachComponent(BaseComponent* component)
{
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