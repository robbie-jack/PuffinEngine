#pragma once

#ifndef ECS_H
#define ECS_H

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
#include <string_view>

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
					entityNames[entity] = "";
					entityDeletionFlags[entity] = false;
				}
			}

			void Init(std::set<Entity> entities)
			{
				for (Entity entity = 1; entity < MAX_ENTITIES; entity++)
				{
					// This entity was not active in loaded scene file, insert into queue as normal
					if (entities.find(entity) == entities.end())
					{
						availableEntities.push(entity);
					}
					// This entity was active in loaded scene file, insert into activeEntities set
					else
					{
						activeEntities.insert(entity);
					}

					entityNames[entity] = "";
					entityDeletionFlags[entity] = false;
				}
			}

			void Cleanup()
			{
				while (!availableEntities.empty())
				{
					availableEntities.pop();
				}

				activeEntities.clear();
				activeEntityCount = 0;
			}

			Entity CreateEntity()
			{
				assert(activeEntityCount < MAX_ENTITIES && "Max number of allowed entities reached");

				// Get next available ID from queue
				Entity entity = availableEntities.front();
				availableEntities.pop();
				activeEntityCount++;

				activeEntities.insert(entity);
				entityNames[entity] = "New Entity";

				return entity;
			}

			void DestroyEntity(Entity entity)
			{
				assert(entity < MAX_ENTITIES && "Entity out of range");

				// Reset signature for this entity
				entitySignatures[entity].reset();
				entityNames[entity] = "";
				entityDeletionFlags[entity] = false;

				activeEntities.erase(entity);

				// Add id to back of queue
				availableEntities.push(entity);
				activeEntityCount--;
			}

			void SetName(Entity entity, std::string name)
			{
				assert(entity < MAX_ENTITIES && "Entity out of range");

				// Update Entity Name
				entityNames[entity] = name;
			}

			std::string GetName(Entity entity)
			{
				assert(entity < MAX_ENTITIES && "Entity out of range");

				// Return Entity Name
				return entityNames[entity];
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

			void MarkToDelete(Entity entity)
			{
				assert(entity < MAX_ENTITIES && "Entity out of range");

				// Mark this entity for deletion so all its component can be cleaned up by the relevant systems
				entityDeletionFlags[entity] = true;
			}

			bool IsDeleted(Entity entity)
			{
				assert(entity < MAX_ENTITIES && "Entity out of range");

				return entityDeletionFlags[entity];
			}

			std::set<Entity> GetActiveEntities()
			{
				return activeEntities;
			}

		private:

			std::queue<Entity> availableEntities;
			std::set<Entity> activeEntities;
			std::array<std::string, MAX_ENTITIES> entityNames;
			std::array<Signature, MAX_ENTITIES> entitySignatures;
			std::array<bool, MAX_ENTITIES> entityDeletionFlags;

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

		template<typename ComponentT>
		class ComponentArray : public IComponentArray
		{
		public:

			ComponentT& AddComponent(Entity entity)
			{
				assert(entityToIndexMap.find(entity) == entityToIndexMap.end() && "Entity already has a component of this type");

				// Insert new component at end of array
				size_t newIndex = arraySize;
				entityToIndexMap[entity] = newIndex;
				indexToEntityMap[newIndex] = entity;
				arraySize++;

				return componentArray[newIndex];
			}

			void AddComponent(Entity entity, ComponentT component)
			{
				assert(entityToIndexMap.find(entity) == entityToIndexMap.end() && "Entity already has a component of this type");

				// Insert new component at end of array
				size_t newIndex = arraySize;
				entityToIndexMap[entity] = newIndex;
				indexToEntityMap[newIndex] = entity;
				componentArray[newIndex] = component;
				arraySize++;
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

				//delete componentArray[indexOfLastComponent];
				componentArray[indexOfLastComponent] = {};

				arraySize--;
			}

			ComponentT& GetComponent(Entity entity)
			{
				assert(entityToIndexMap.find(entity) != entityToIndexMap.end() && "Retrieving non-existent component.");

				// Return reference to component
				return componentArray[entityToIndexMap[entity]];
			}

			bool HasComponent(Entity entity)
			{
				bool hasComponent = false;

				if (entityToIndexMap.find(entity) != entityToIndexMap.end())
				{
					hasComponent = true;
				}

				return hasComponent;
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
			std::array<ComponentT, MAX_ENTITIES> componentArray;

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

			void Cleanup()
			{
				componentTypes.clear();
				componentArrays.clear();
			}

			template<typename ComponentT>
			void RegisterComponent()
			{
				const char* typeName = typeid(ComponentT).name();

				assert(componentTypes.find(typeName) == componentTypes.end() && "Registering component type more than once");

				// Add new component type to component type map
				componentTypes.insert({ typeName, nextComponentType });

				// Create ComponentT Array Pointers
				std::shared_ptr<ComponentArray<ComponentT>> array = std::make_shared<ComponentArray<ComponentT>>();

				// Cast ComponentArray pointer to IComponentArray and add to component arrays map
				componentArrays.insert({typeName, std::static_pointer_cast<IComponentArray>(array) });

				// Increment next component type
				nextComponentType++;
			}

			template<typename ComponentT>
			ComponentType GetComponentType()
			{
				const char* typeName = typeid(ComponentT).name();

				assert(componentTypes.find(typeName) != componentTypes.end() && "ComponentType not registered before use");

				// Return this components type - used for creating signatures
				return componentTypes[typeName];
			}

			template<typename ComponentT>
			ComponentT& AddComponent(Entity entity)
			{
				// Add a component to array for this entity
				return GetComponentArray<ComponentT>()->AddComponent(entity);
			}

			template<typename ComponentT>
			void AddComponent(Entity entity, ComponentT component)
			{
				// Add a component to array for this entity
				GetComponentArray<ComponentT>()->AddComponent(entity, component);
			}

			template<typename ComponentT>
			void RemoveComponent(Entity entity)
			{
				// Remove component from array for this entity
				GetComponentArray<ComponentT>()->RemoveComponent(entity);
			}

			template<typename ComponentT>
			ComponentT& GetComponent(Entity entity)
			{
				// Get reference to component for this entity
				return GetComponentArray<ComponentT>()->GetComponent(entity);
			}

			template<typename ComponentT>
			bool HasComponent(Entity entity)
			{
				assert(componentTypes.find(typeName) != componentTypes.end() && "ComponentType not registered before use");

				// Return true of array has component for this entity
				return GetComponentArray<ComponentT>()->HasComponent(entity);
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

			// ComponentType type to be assigned to netx registered component
			ComponentType nextComponentType;

			template<typename ComponentT>
			std::shared_ptr<ComponentArray<ComponentT>> GetComponentArray()
			{
				const char* typeName = typeid(ComponentT).name();

				assert(componentTypes.find(typeName) != componentTypes.end() && "ComponentType not registered before use");

				return std::static_pointer_cast<ComponentArray<ComponentT>>(componentArrays[typeName]);
			}
		};

		//////////////////////////////////////////////////
		// System
		//////////////////////////////////////////////////

		class World;

		typedef std::unordered_map<std::string_view, std::set<Entity>> EntityMap;

		class System
		{
		public:
			EntityMap entityMap;
			std::shared_ptr<World> world;
			Entity entityToDelete;
		};

		//////////////////////////////////////////////////
		// System Manager
		//////////////////////////////////////////////////

		// Stores all signatures used by a system
		typedef std::unordered_map<std::string_view, Signature> SignatureMap;

		class SystemManager
		{
		public:

			void Cleanup()
			{
				signatureMap.clear();

				systems.clear();
			}

			template<typename SystemT>
			std::shared_ptr<SystemT> RegisterSystem(std::shared_ptr<World> world)
			{
				const char* typeName = typeid(SystemT).name();

				assert(mSystems.find(typeName) == mSystems.end() && "Registering system more than once.");

				// Create New Signature Map for this System
				signatureMap.insert({ typeName, SignatureMap() });

				// Create and return pointer to system
				std::shared_ptr<SystemT> system = std::make_shared<SystemT>();
				std::static_pointer_cast<System>(system)->world = world;
				systems.insert({ typeName, std::static_pointer_cast<System>(system) });
				return system;
			}

			template<typename SystemT>
			void SetSignature(std::string_view signatureName, Signature signature)
			{
				const char* typeName = typeid(SystemT).name();

				assert(mSystems.find(typeName) != mSystems.end() && "System used before registered.");

				// Insert New Signature for this System
				signatureMap.at(typeName).insert({ signatureName, signature });
				
				// Insert New Set for this System
				systems.at(typeName)->entityMap.insert({ signatureName, std::set<Entity>() });
			}

			void EntityDestroyed(Entity entity)
			{
				// Erase destroyed entity from all system lists
				// Entities is a set so no check needed
				for (auto const& pair : systems)
				{
					auto const& system = pair.second;

					// Erase Entity for every set in System
					for (auto const& map_pair : system->entityMap)
					{
						auto const& signatureName = map_pair.first;

						system->entityMap.at(signatureName).erase(entity);
					}
				}
			}

			void EntitySignatureChanged(Entity entity, Signature entitySignature)
			{
				// Notify each system that entity signature has changed
				for (auto const& pair : systems)
				{
					auto const& type = pair.first;
					auto const& system = pair.second;
					auto const& systemMap = signatureMap[type];

					// Iterate over each signature of of the system
					for (auto const& map : systemMap)
					{
						auto const& signatureName = map.first;
						auto const& systemSignature = map.second;

						// Entity signature matches system signature - insert into matching set
						if ((entitySignature & systemSignature) == systemSignature)
						{
							system->entityMap.at(signatureName).insert(entity);
						}
						// Entity signature does not match system signature - erase from set
						else
						{
							system->entityMap.at(signatureName).erase(entity);
						}
					}
				}
			}

		private:

			// Map from system type string pointer to signature
			//std::unordered_map<const char*, Signature> signatures;
			std::unordered_map<const char*, SignatureMap> signatureMap;

			// Map from system type string to system pointer
			std::unordered_map<const char*, std::shared_ptr<System>> systems;
		};

		//////////////////////////////////////////////////
		// ECS World
		//////////////////////////////////////////////////

		class World
		{
		public:

			void Init()
			{
				// Create pointers to each manager
				componentManager = std::make_unique<ComponentManager>();
				entityManager = std::make_unique<EntityManager>();
				systemManager = std::make_unique<SystemManager>();
			}

			void Update()
			{
				// Remove all entities marked for deletion
				for (auto entity : entityManager->GetActiveEntities())
				{
					if (IsDeleted(entity))
					{
						DestroyEntity(entity);
					}
				}
			}

			// Reset Entities and Components, Leave Systems Intact
			void Reset()
			{
				for (auto entity : entityManager->GetActiveEntities())
				{
					DestroyEntity(entity);
				}

				entityManager->Cleanup();
			}

			// Cleanup Entire ECS
			void Cleanup()
			{
				for (auto entity : entityManager->GetActiveEntities())
				{
					DestroyEntity(entity);
				}

				componentManager->Cleanup();
				entityManager->Cleanup();
				systemManager->Cleanup();

				componentManager.reset();
				entityManager.reset();
				systemManager.reset();
			}

			// Entity Methods
			void InitEntitySystem(std::set<Entity> entities)
			{
				entityManager->Init(entities);
			}

			Entity CreateEntity()
			{
				return entityManager->CreateEntity();
			}

			void DestroyEntity(Entity entity)
			{
				entityManager->DestroyEntity(entity);

				componentManager->EntityDestroyed(entity);

				systemManager->EntityDestroyed(entity);
			}

			void SetEntityName(Entity entity, std::string name)
			{
				entityManager->SetName(entity, name);
			}

			std::string GetEntityName(Entity entity)
			{
				return entityManager->GetName(entity);
			}

			Signature GetEntitySignature(Entity entity)
			{
				return entityManager->GetSignature(entity);
			}

			void MarkToDelete(Entity entity)
			{
				entityManager->MarkToDelete(entity);
			}

			bool IsDeleted(Entity entity)
			{
				return entityManager->IsDeleted(entity);
			}

			std::set<Entity> GetActiveEntities()
			{
				return entityManager->GetActiveEntities();
			}

			// Component Methods
			template<typename ComponentT>
			void RegisterComponent()
			{
				componentManager->RegisterComponent<ComponentT>();
			}

			template<typename ComponentT>
			ComponentT& AddComponent(Entity entity)
			{
				ComponentT& comp = componentManager->AddComponent<ComponentT>(entity);

				auto signature = entityManager->GetSignature(entity);
				signature.set(componentManager->GetComponentType<ComponentT>(), true);
				entityManager->SetSignature(entity, signature);

				systemManager->EntitySignatureChanged(entity, signature);

				return comp;
			}

			template<typename ComponentT>
			void AddComponent(Entity entity, ComponentT component)
			{
				componentManager->AddComponent<ComponentT>(entity, component);

				auto signature = entityManager->GetSignature(entity);
				signature.set(componentManager->GetComponentType<ComponentT>(), true);
				entityManager->SetSignature(entity, signature);

				systemManager->EntitySignatureChanged(entity, signature);
			}

			template<typename ComponentT>
			void RemoveComponent(Entity entity)
			{
				componentManager->RemoveComponent<ComponentT>(entity);

				auto signature = entityManager->GetSignature(entity);
				signature.set(componentManager->GetComponentType<ComponentT>(), false);
				entityManager->SetSignature(entity, signature);

				systemManager->EntitySignatureChanged(entity, signature);
			}

			template<typename ComponentT>
			ComponentT& GetComponent(Entity entity)
			{
				return componentManager->GetComponent<ComponentT>(entity);
			}

			template<typename ComponentT>
			ComponentType GetComponentType()
			{
				return componentManager->GetComponentType<ComponentT>();
			}

			template<typename ComponentT>
			bool HasComponent(Entity entity)
			{
				return componentManager->HasComponent<ComponentT>(entity);
			}

			// System Methods

			template<typename SystemT>
			std::shared_ptr<SystemT> RegisterSystem()
			{
				std::shared_ptr<World> worldPtr{ this };
				return systemManager->RegisterSystem<SystemT>(worldPtr);
			}

			template<typename SystemT>
			void SetSystemSignature(std::string_view signatureName, Signature signature)
			{
				systemManager->SetSignature<SystemT>(signatureName, signature);

				// Update System's local entity list with any new entities
				for (Entity entity : entityManager->GetActiveEntities())
				{
					systemManager->EntitySignatureChanged(entity, entityManager->GetSignature(entity));
				}
			}

		private:

			std::unique_ptr<ComponentManager> componentManager;
			std::unique_ptr<EntityManager> entityManager;
			std::unique_ptr<SystemManager> systemManager;

		};
	}
}

#endif // ECS_H