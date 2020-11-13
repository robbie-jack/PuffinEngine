#pragma once

#include <cstdint>
#include <bitset>
#include <queue>
#include <array>
#include <cassert>
#include <unordered_map>
#include <vector>
#include <set>
#include <memory>
#include <typeinfo>

namespace Puffin
{
	namespace ECS
	{
		typedef uint32_t Entity;
		const Entity MAX_ENTITIES = 5000;

		static const Entity INVALID_ENTITY = 0;

		typedef uint8_t ComponentType;
		const ComponentType MAX_COMPONENTS = 32;

		typedef std::bitset<MAX_COMPONENTS> Signature;

		//////////////////////////////////////////////////
		// Entity Manager
		//////////////////////////////////////////////////

		class EntityManager
		{
		public:

			EntityManager()
			{
				for (Entity entity = 1; entity < MAX_ENTITIES; entity++)
				{
					availableEntities.push(entity);
				}
			}

			Entity CreateEntity()
			{
				assert(activeEntityCount < MAX_ENTITIES && "Max number of allowed entities reached");

				// Get next available ID from queue
				Entity entity = availableEntities.front();
				availableEntities.pop();
				activeEntityCount++;

				activeEntities.insert(entity);

				return entity;
			}

			void DestroyEntity(Entity entity)
			{
				assert(entity < MAX_ENTITIES && "Entity out of range");

				// Reset signature for this entity
				entitySignatures[entity].reset();

				activeEntities.erase(entity);

				// Add id to back of queue
				availableEntities.push(entity);
				activeEntityCount--;
			}

			void SetSignature(Entity entity, Signature signature)
			{
				assert(entity < MAX_ENTITIES && "Entity out of range");

				// Update this entity's signature
				entitySignatures[entity] = signature;
			}

			Signature GetSignature(Entity entity)
			{
				assert(entity < MAX_ENTITIES && "Entity out of range");

				// Get entity signature
				return entitySignatures[entity];
			}

			std::set<Entity> GetActiveEntities()
			{
				return activeEntities;
			}

		private:

			std::queue<Entity> availableEntities;
			std::set<Entity> activeEntities;
			std::array<Signature, MAX_ENTITIES> entitySignatures;

			uint32_t activeEntityCount;
		};

		//////////////////////////////////////////////////
		// Component Array
		//////////////////////////////////////////////////

		class IComponentArray
		{
		public:
			virtual ~IComponentArray() = default;
			virtual void EntityDestroyed(Entity entity) = 0;
		};

		template<typename Component>
		class ComponentArray : IComponentArray
		{
		public:

			Component& AddComponent(Entity entity)
			{
				assert(entityToIndexMap.find(entity) == entityToIndexMap.end() && "Entity already has a component of this type");

				Component component;

				// Insert new component at end of array
				size_t newIndex = arraySize;
				entityToIndexMap[entity] = newIndex;
				indexToEntityMap[newIndex] = entity;
				componentArray[newIndex] = component;
				arraySize++;

				return component;
			}

			void RemoveComponent(Entity entity)
			{
				assert(mEntityToIndexMap.find(entity) != mEntityToIndexMap.end() && "Removing non-existent component.");

				// Copy component at end of array into deleted components spaced to maintain packed araay
				size_t indexOfRemovedComponent = entityToIndexMap[entity];
				size_t indexOfLastComponent = arraySize - 1;
				componentArray[indexOfRemovedComponent] = componentArray[indexOfLastComponent];

				// Update map to point to components new location
				Entity entityOfLastComponent = indexToEntityMap[indexOfLastComponent];
				entityToIndexMap[entityOfLastComponent] = indexOfRemovedComponent;
				indexToEntityMap[indexOfRemovedComponent] = entityOfLastComponent;

				entityToIndexMap.erase(entity);
				indexToEntityMap.erase(indexOfLastComponent);

				arraySize--;
			}

			Component& GetComponent(Entity entity)
			{
				assert(mEntityToIndexMap.find(entity) != mEntityToIndexMap.end() && "Retrieving non-existent component.");

				// Return reference to component
				return componentArray[entityToIndexMap[entity]];
			}

			void EntityDestroyed(Entity entity) override
			{
				if (entityToIndexMap.find(entity) != entityToIndexMap.end())
				{
					// Remove entities component if it existed
					RemoveComponent(entity);
				}
			}


		private:

			// Packed array of components
			std::array<Component, MAX_ENTITIES> componentArray;

			// Map from entity to array
			std::unordered_map<Entity, size_t> entityToIndexMap;

			// Map from array to entity
			std::unordered_map<size_t, Entity> indexToEntityMap;

			// Size of valid entries in array
			size_t arraySize;
		};

		//////////////////////////////////////////////////
		// Component Manager
		//////////////////////////////////////////////////

		class ComponentManager
		{
		public:

			template<typename Component>
			void RegisterComponent()
			{
				const char* typeName = typeid(Component).name();

				assert(componentTypes.find(typeName) == componentTypes.end() && "Registering component type more than once");

				// Add new component type to component type map
				componentTypes.insert({ typeName, nextComponentType });

				std::shared_ptr<ComponentArray<Component>> array;

				std::pair<const char*, std::shared_ptr<IComponentArray>> pair;
				pair.first = typeName;
				pair.second = std::dynamic_pointer_cast<IComponentArray>(array);

				// Create ComponentArray pointer and add to component arrays map
				componentArrays.insert(pair);

				// Increment next component type
				nextComponentType++;
			}

			template<typename Component>
			ComponentType GetComponentType()
			{
				const char* typeName = typeid(Component).name();

				assert(componentTypes.find(typeName) != componentTypes.end() && "Component not registered before use");

				// Return this components type - used for creating signatures
				return componentTypes[typeName];
			}

			template<typename Component>
			Component& AddComponent(Entity entity)
			{
				// Add a component to array for this entity
				return GetComponentArray<Component>()->AddComponent(entity);
			}

			template<typename Component>
			void RemoveComponent(Entity entity)
			{
				// Remove component from array for this entity
				GetComponentArray<Component>()->RemoveComponent(entity);
			}

			template<typename Component>
			Component& GetComponent(Entity entity)
			{
				// Get reference to component for this entity
				return GetComponentArray<Component>()->GetComponent(entity);
			}

			void EntityDestroyed(Entity entity)
			{
				// Notify each component array that an entity has been destroyed
				// If array has component for this entity, remove it
				for (auto const& pair : componentArrays)
				{
					auto const& componentArray = pair.second;

					componentArray->EntityDestroyed(entity);
				}
			}

		private:

			// Map from type string pointer to component type
			std::unordered_map<const char*, ComponentType> componentTypes;

			// Map from type string pointer to component array
			std::unordered_map<const char*, std::shared_ptr<IComponentArray>> componentArrays;

			// Component type to be assigned to netx registered component
			ComponentType nextComponentType;

			template<typename Component>
			std::shared_ptr<ComponentArray<Component>> GetComponentArray()
			{
				const char* typeName = typeid(Component).name();

				assert(componentTypes.find(typeName) != componentTypes.end() && "Component not registered before use");

				return std::static_pointer_cast<ComponentArray<Component>>(componentArrays[typeName]);
			}
		};
	}
}