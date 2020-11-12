#pragma once

#include <vector>
#include <unordered_map>
#include <memory>

namespace Puffin
{
	namespace ECS
	{
		typedef uint32_t Entity;
		typedef uint32_t ComponentType;

		////////////////////////////////////////
		// Component Array
		////////////////////////////////////////

		class IComponentArray
		{
		public:
			virtual ~IComponentArray() = default;
			virtual void EntityDestroyed(Entity entity) = 0;
		};

		template<typename Component>
		class ComponentArray : public IComponentArray
		{
			Component& CreateComponent(Entity entity);
			Component* GetComponent(Entity entity);
			void RemoveComponent(Entity entity);

			inline bool Contains(Entity entity) { return lookup.find(entity) != lookup.end(); }
			inline const size_t GetCount() { return components.size(); };
			inline Entity GetEntity(size_t index) { return entities[index]; };

			Component& operator[](size_t index) { return components[index]; };

			void EntityDestoyed(Entity entity) override;

		private:

			std::vector<Component> components;
			std::vector<Entity> entities;
			std::unordered_map<Entity, size_t> lookup;

		};

		////////////////////////////////////////
		// Component Manager
		////////////////////////////////////////

		class ComponentManager
		{
		public:

			template<typename Component>
			void RegisterComponent();

			template<typename Component>
			ComponentType GetComponentType();

			template<typename Component>
			Component& CreateComponent(Entity entity);

			template<typename Component>
			Component* GetComponent(Entity entity);

			template<typename Component>
			void RemoveComponent(Entity entity);

			void EntityDestroyed(Entity entity);

			template<typename Component>
			std::shared_ptr<ComponentArray<Component>> GetComponentArray();

		private:

			ComponentType nextComponentType = 0;

			std::unordered_map<const char*, ComponentType> componentTypes;
			std::unordered_map<const char*, std::shared_ptr<IComponentArray>> componentArrays;
		};
	}
}