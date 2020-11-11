#pragma once

#include <vector>
#include <unordered_map>

namespace Puffin
{
	namespace ECS
	{
		typedef uint32_t Entity;

		////////////////////////////////////////
		// Component Array
		////////////////////////////////////////

		template<typename Component>
		class ComponentArray
		{
			Component& CreateComponent(Entity entity);
			Component* GetComponent(Entity entity);
			void RemoveComponent(Entity entity);

			inline bool Contains(Entity entity) { return lookup.find(entity) != lookup.end(); }
			inline const size_t GetCount() { return components.size(); };
			inline Entity GetEntity(size_t index) { return entities[index]; };

			Component& operator[](size_t index) { return components[index]; };

		private:

			std::vector<Component> components;
			std::vector<Entity> entities;
			std::unordered_map<Entity, size_t> lookup;

		};
	}
}