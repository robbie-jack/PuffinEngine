#include "ComponentManager.h"

#include <cassert>
#include <typeinfo>

namespace Puffin
{
	namespace ECS
	{
		////////////////////////////////////////
		// Component Array
		////////////////////////////////////////

		template<typename Component>
		Component& ComponentArray<Component>::CreateComponent(Entity entity)
		{
			// 0 is reserved for invalid entities
			assert(entity != 0 && "Invalid Entity");

			assert(lookup.find(entity) == lookup.end() && "Component Type already exists for this entity");

			assert(entities.size() == components.size() && "Number of entities must match components");
			assert(lookup.size() == components.size() && "Number of lookups must match components");

			// Update lookup map
			lookup[entity] = components.size();

			// Add new component to array
			components.push_back(Component());

			// Add entity to array
			entities.push_back(entity);

			return components.back();
		}

		template<typename Component>
		Component* ComponentArray<Component>::GetComponent(Entity entity)
		{
			auto it = lookup.find(entity);
			Component* comp = nullptr;

			if (it != lookup.end())
			{
				comp = &components[it->second];
			}

			return comp;
		}

		template<typename Component>
		void ComponentArray<Component>::RemoveComponent(Entity entity)
		{
			auto it = lookup.find(entity);

			if (it != lookup.end())
			{
				// Index into components and entities array
				const size_t index = it->second;
				const Entity entity = entities[index];

				if (index < components.size() - 1)
				{
					// Swap out dead element with last one
					components[index] = std::move(components.back());
					entities[index] = entities.back();

					// Update lookup map
					lookup[entities[index]] = index;
				}

				// Shrink container
				components.pop_back();
				entities.pop_back();
				lookup.erase(entity);
			}
		}

		template<typename Component>
		void ComponentArray<Component>::EntityDestoyed(Entity entity)
		{
			if (lookup.find(entity) != lookup.end())
			{
				RemoveComponent(entity);
			}
		}

		////////////////////////////////////////
		// Component Manager
		////////////////////////////////////////

		template<typename Component>
		void ComponentManager::RegisterComponent()
		{
			const char* typeName = typeid(Component).name();

			assert(componentTypes.find(typeName) == componentTypes.end() && "Registering component type more than once");

			// Add this component type to component type map
			componentTypes.insert({ typeName, nextComponentType });

			// Create new Component Array
			componentArrays.insert({ typeName, std::make_shared<ComponentArray<Component>>()});

			nextComponentType++;
		}

		template<typename Component>
		ComponentType ComponentManager::GetComponentType()
		{
			const char* typeName = typeid(Component).name();

			assert(componentTypes.find(typeName) != componentTypes.end() && "Component type not registered before use");

			// Return component's type
			return componentTypes[typeName];
		}

		template<typename Component>
		Component& ComponentManager::CreateComponent(Entity entity)
		{
			// Create new component for entity in array
			return GetComponentArray<Component>()->CreateComponent(entity);
		}

		template<typename Component>
		Component* ComponentManager::GetComponent(Entity entity)
		{
			// Get Component for this entity from Array, if it exists
			return GetComponentArray<Component>()->GetComponent(entity);
		}

		template<typename Component>
		void ComponentManager::RemoveComponent(Entity entity)
		{
			// Remove component fro this entity from array
			GetComponentArray<Component>()->RemoveComponent(entity);
		}

		void ComponentManager::EntityDestroyed(Entity entity)
		{
			// Remove all components for this entity
			for (auto const& pair : componentArrays)
			{
				auto const& array = pair.second;

				array->EntityDestroyed(entity);
			}
		}

		template<typename Component>
		std::shared_ptr<ComponentArray<Component>> ComponentManager::GetComponentArray()
		{
			const char* typeName = typeid(Component).name();

			assert(componentTypes.find(typeName) != componentTypes.end() && "Component type not registered before use");

			return std::static_pointer_cast<ComponentArray<Component>>(componentArrays[typeName]);
		}
	}
}