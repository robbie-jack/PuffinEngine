#include "ComponentManager.h"

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
			assert(entity != 0 & "Invalid Entity");

			assert(lookup.find(entity) == lookup.end() & "Component Type already exists for this entity");

			assert(entities.size() == components.size() & "Number of entities must match components");
			assert(lookup.size() == components.size() & "Number of lookups must match components");

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
	}
}