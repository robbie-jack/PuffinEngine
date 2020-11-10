#include "ECS.h"

#include <typeinfo>

namespace Puffin 
{
	namespace ECS
	{
		////////////////////////////////////////
		// Entity Manager
		////////////////////////////////////////

		EntityManager::EntityManager()
		{
			// Intialise avilable entities queue with all possible entity IDs
			for (Entity entity = 0; entity < MAX_ENTITIES; entity++)
			{
				availableEntities.push(entity);
			}
		}

		EntityManager::~EntityManager()
		{

		}

		Entity EntityManager::CreateEntity()
		{
			assert(livingEntityCount < MAX_ENTITIES && "Too many entities exist");

			// Take ID from front of queue
			Entity id = availableEntities.front();
			availableEntities.pop();
			livingEntityCount++;

			return id;
		}

		void EntityManager::DestroyEntity(Entity entity)
		{
			assert(entity < MAX_ENTITIES && "Entity out of range");

			// Invalidate destroyed entities signature
			entitySignatures[entity].reset();

			// Add destroyed ID to back of queue
			availableEntities.push(entity);
			livingEntityCount--;
		}

		void EntityManager::SetSignature(Entity entity, Signature signature)
		{
			assert(entity < MAX_ENTITIES && "Entity out of range");

			// Enter entity signature into array
			entitySignatures[entity] = signature;
		}

		Signature EntityManager::GetSignature(Entity entity)
		{
			assert(entity < MAX_ENTITIES && "Entity out of range");

			// Get entity signature from array
			return entitySignatures[entity];
		}

		////////////////////////////////////////
		// Component Array
		////////////////////////////////////////

		template<typename T>
		void ComponentArray<T>::InsertData(Entity entity, T component)
		{
			assert(entityToIndexMap.find(entity) == entityToIndexMap.end() && "Component added to same entity more than once");

			// Add entry at end and update maps
			size_t newIndex = entrySize;
			entityToIndexMap[entity] = newIndex;
			indexToEntityMap[newIndex] = entity;
			componentArray[newIndex] = component;
			entrySize++;
		}

		template<typename T>
		void ComponentArray<T>::RemoveData(Entity entity)
		{
			assert(entityToIndexMap.find(entity) != entityToIndexMap.end() && "Removing non-existent component");

			// Copy element at end into deleted elements place
			size_t indexOfRemovedEntity = entityToIndexMap[entity];
			size_t indexOfLastElement = entrySize - 1;
			componentArray[indexOfRemovedEntity] = componentArray[indexOfLastElement];

			// Update map to point to moved spot
			Entity entityOfLastElement = indexToEntityMap[indexOfLastElement];
			entityToIndexMap[entityOfLastElement] = indexOfRemovedEntity;
			indexToEntityMap[indexOfRemovedEntity] = entityOfLastElement;

			entityToIndexMap.erase(entity);
			entityToIndexMap.erase(indexOfLastElement);

			entrySize--;
		}

		template<typename T>
		T& ComponentArray<T>::GetData(Entity entity)
		{
			assert(entityToIndexMap.find(entity) != entityToIndexMap.end() && "Removing non-existent component");

			// Return a reference to entity's component
			return componentArray[entityToIndexMap[entity]];
		}

		template<typename T>
		void ComponentArray<T>::EntityDestroyed(Entity entity)
		{
			if (entityToIndexMap.find(entity) != entityToIndexMap.end())
			{
				// Remove entities component if it existed
				RemoveData(entity);
			}
		}

		////////////////////////////////////////
		// Component Manager
		////////////////////////////////////////

		template<typename T>
		void ComponentManager::RegisterComponent()
		{
			const char* typeName = typeid(T).name();

			assert(componentTypes.find(typeName) == componentTypes.end() "Rigistering component type more rhan once");

			// Add this component type to the type map
			componentTypes.insert({ typeName, nextComponentType });

			// Create a ComponentArray pointer and add it to the component arrays map
			componentArrays.insert({ typeName, std::make_shared<ComponentArray<T>>() });

			// Increment next component type
			nextComponentType++;
		}

		template<typename T>
		ComponentType ComponentManager::GetComponentType()
		{
			const char* typeName = typeid(T).name();

			assert(componentTypes.find(typeName) != componentTypes.end() && "Component not registered before use");

			// Return this component type
			return componentTypes[typeName];
		}

		template<typename T>
		void ComponentManager::AddComponent(Entity entity, T component)
		{
			// Add component to array
			GetComponentArray<T>()->InsertData(entity, component);
		}

		template<typename T>
		T& ComponentManager::GetComponent(Entity entity)
		{
			// Remove component from array
			GetComponentArray<T>()->RemoveData(entity);
		}

		void ComponentManager::EntityDestroyed(Entity entity)
		{
			// Notify each component array an enity has been destroyed, and remove components for that entity
			for (auto const& pair : componentArrays)
			{
				auto const& componentArray = pair.second;
				componentArray->EntityDestroyed(entity);
			}
		}
	}
}