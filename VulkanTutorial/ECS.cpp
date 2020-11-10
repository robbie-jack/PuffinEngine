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
		void ComponentManager::RemoveComponent(Entity entity)
		{
			// Remove component from array
			GetComponentArray<T>()->RemoveData(entity);
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

		////////////////////////////////////////
		// System Manager
		////////////////////////////////////////

		template<typename T>
		std::shared_ptr<T> SystemManager::RegisterSystem()
		{
			const char* typeName = typeid(T).name();

			assert(systems.find(typeName) == systems.end() && "Registering system more than once");

			// Create and return a ponter to system so it can be used externally
			auto system = std::make_shared<T>();
			systems.insert({ typeName, system });
			return system;
		}

		template<typename T>
		void SystemManager::SetSignature(Signature signature)
		{
			const char* typeName = typeid(T).name();

			assert(systems.find(typeName) != systems.end() && "System used before being registered.");

			// Set system signature
			signatures.insert({ typeName, signature });
		}

		void SystemManager::EntityDestroyed(Entity entity)
		{
			// Erase destroyed entity from all systems
			for (auto const& pair : systems)
			{
				auto const system = pair.second;

				system->Remove(entity);
			}
		}

		void SystemManager::EntitySignatureChanged(Entity entity, Signature entitySignature)
		{
			// Notify eahc system that entity signature has changed
			for (auto const& pair : systems)
			{
				auto const& type = pair.first;
				auto const& system = pair.second;
				auto const& systemSignature = signatures[type];

				// Entity signature matches system signature - insert into set
				if ((entitySignature & systemSignature) == systemSignature)
				{
					system->Add(entity);
				}
				// Signatures do not match - erase from set
				else
				{
					system->Remove(entity);
				}
			}
		}

		////////////////////////////////////////
		// ECS Coordinator
		////////////////////////////////////////

		// Entity Methods
		void Coordinator::Init()
		{
			// Create pointers to each manager
			componentManager = std::make_unique<ComponentManager>();
			entityManager = std::make_unique<EntityManager>();
			systemManager = std::make_unique<SystemManager>();
		}

		Entity Coordinator::CreateEntity()
		{
			return entityManager->CreateEntity();
		}

		void Coordinator::DestroyEntity(Entity entity)
		{
			entityManager->DestroyEntity(entity);

			componentManager->EntityDestroyed(entity);

			systemManager->EntityDestroyed(entity);
		}

		// Component Methods
		template<typename T>
		void Coordinator::RegisterComponent()
		{
			componentManager->RegisterComponent<T>();
		}

		template<typename T>
		void Coordinator::AddComponent(Entity entity, T component)
		{
			componentManager->AddComponent<T>(entity, component);

			auto signature = entityManager->GetSignature(entity);
			signature.set(componentManager->GetComponentType<T>(), true);
			entityManager->SetSignature(entity, signature);

			systemManager->EntitySignatureChanged(entity, signature);
		}

		template<typename T>
		void Coordinator::RemoveComponent(Entity entity)
		{
			componentManager->RemoveComponent<T>(entity);

			auto signature = entityManager->GetSignature(entity);
			signature.set(componentManager->GetComponentType<T>(), false);
			entityManager->SetSignature(entity, signature);

			systemManager->EntitySignatureChanged(entity, signature);
		}

		template<typename T>
		T& Coordinator::GetComponent(Entity entity)
		{
			return componentManager->GetComponent<T>(entity);
		}

		template<typename T>
		ComponentType Coordinator::GetComponentType()
		{
			return componentManager->GetComponentType<T>();
		}

		// System Methods
		template<typename T>
		std::shared_ptr<T> Coordinator::RegisterSystem()
		{
			return systemManager->RegisterSystem<T>();
		}

		template<typename T>
		void Coordinator::SetSystemSignature(Signature signature)
		{
			systemManager->SetSignature<T>(signature);
		}
	}
}