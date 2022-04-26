#pragma once

#include "ECS\EventManager.h"
#include "ECS\System.h"

#include "Types/PackedArray.h"
#include "Types/ComponentFlags.h"

#include <cstdint>
#include <bitset>
#include <queue>
#include <array>
#include <cassert>
#include <map>
#include <unordered_map>
#include <vector>
#include <set>
#include <memory>
#include <typeinfo>
#include <string_view>
#include <bitset>

namespace Puffin::ECS
{
	typedef uint32_t Entity;
	const Entity MAX_ENTITIES = 5000;

	static const Entity INVALID_ENTITY = 0;

	typedef uint8_t ComponentType;
	const ComponentType MAX_COMPONENTS = 255;

	typedef std::bitset<MAX_COMPONENTS> Signature;

	typedef uint8_t FlagType;
	const FlagType MAX_FLAGS = 255;

	//////////////////////////////////////////////////
	// Entity Manager
	//////////////////////////////////////////////////

	class EntityManager
	{
	public:

		EntityManager()
		{
			activeEntityCount = 0;
			nextFlagType = 0;
		}

		~EntityManager()
		{
			Cleanup();
			flagTypes.clear();
			flagSets.clear();
			flagDefaults.clear();
		}

		void Init()
		{
			if (bInitialized)
			{
				return;
			}

			for (Entity entity = 1; entity < MAX_ENTITIES; entity++)
			{
				availableEntities.push(entity);

				entityNames[entity] = "";
				
				// Set all flags back to default
				for (auto& pair : flagSets)
				{
					pair.second[entity] = flagDefaults[pair.first];
				}
			}

			bInitialized = true;
		}

		void Init(std::set<Entity>& entities)
		{
			if (bInitialized)
			{
				return;
			}

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
				
				// Set all flags back to default
				for (auto& pair : flagSets)
				{
					pair.second[entity] = flagDefaults[pair.first];
				}
			}

			bInitialized = true;
		}

		void Cleanup()
		{
			while (!availableEntities.empty())
			{
				availableEntities.pop();
			}

			activeEntities.clear();
			activeEntityCount = 0;
			bInitialized = false;
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

			// Set all flags back to default
			for (auto& pair : flagSets)
			{
				pair.second[entity] = flagDefaults[pair.first];
			}

			return entity;
		}

		void DestroyEntity(Entity entity)
		{
			assert(entity < MAX_ENTITIES && "Entity out of range");

			// Reset signature for this entity
			entitySignatures[entity].reset();
			entityNames[entity] = "";

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

		template<typename FlagT>
		void RegisterEntityFlag(bool flagDefault)
		{
			const char* typeName = typeid(FlagT).name();

			assert(flagTypes.find(typeName) == flagTypes.end() && "Registering flag type more than once");
			assert(nextFlagType < MAX_FLAGS && "Registering more than maximum number of flags");

			// Add new flag type to flag type map
			flagTypes.insert({typeName, nextFlagType});

			// Add new flag bitset
			flagSets[flagTypes[typeName]] = std::bitset<MAX_ENTITIES>();
			flagDefaults[flagTypes[typeName]] = flagDefault;

			nextFlagType++;
		}

		template<typename FlagT>
		bool GetEntityFlag(Entity entity)
		{
			const char* typeName = typeid(FlagT).name();

			assert(activeEntities.count(entity) == 1 && "Entity does not exist");
			assert(flagTypes.find(flagTypeName) != flagTypes.end() && "FlagType not registered before use");

			return flagSets[flagTypes[typeName]][entity];
		}

		template<typename FlagT>
		void SetEntityFlag(Entity entity, bool flag)
		{
			const char* typeName = typeid(FlagT).name();

			assert(activeEntities.count(entity) == 1 && "Entity does not exist");
			assert(flagTypes.find(flagTypeName) != flagTypes.end() && "FlagType not registered before use");

			flagSets[flagTypes[typeName]][entity] = flag;
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

		// FlagType to be assigned to next registered flag
		FlagType nextFlagType;

		// Map from type string pointer to flag type
		std::unordered_map<const char*, FlagType> flagTypes;

		std::unordered_map<FlagType, std::bitset<MAX_ENTITIES>> flagSets;
		std::unordered_map<FlagType, bool> flagDefaults; // What to have each flag type default to

		uint32_t activeEntityCount;

		bool bInitialized = false;
	};

	//////////////////////////////////////////////////
	// Component Array
	//////////////////////////////////////////////////

	class IComponentArray
	{
	public:
		virtual ~IComponentArray() = default;
		virtual void RegisterComponentFlag(FlagType flagType, bool flagDefault) = 0;
		virtual void EntityDestroyed(Entity entity) = 0;
	};

	template<typename ComponentT>
	class ComponentArray : public IComponentArray
	{
	public:

		~ComponentArray()
		{
			flagSets.clear();
			flagDefaults.clear();
		}

		ComponentT& AddComponent(Entity entity)
		{
			assert(!componentArray.Contains(entity) && "Entity already has a component of this type");

			componentArray.Insert(entity, ComponentT());

			// Set all flags back to default
			for (auto& pair : flagSets)
			{
				pair.second[entity] = flagDefaults[pair.first];
			}

			return componentArray[entity];
		}

		void AddComponent(Entity entity, ComponentT component)
		{
			assert(!componentArray.Contains(entity) && "Entity already has a component of this type");

			componentArray.Insert(entity, component);

			// Set all flags back to default
			for (auto& pair : flagSets)
			{
				pair.second[entity] = flagDefaults[pair.first];
			}
		}

		void RemoveComponent(Entity entity)
		{
			assert(componentArray.Contains(entity) && "Removing non-existent component.");

			componentArray.Erase(entity);
		}

		ComponentT& GetComponent(Entity entity)
		{
			assert(componentArray.Contains(entity) && "Retrieving non-existent component.");

			return componentArray[entity];
		}

		bool HasComponent(Entity entity)
		{
			return componentArray.Contains(entity);
		}

		void RegisterComponentFlag(FlagType flagType, bool flagDefault) override
		{
			flagSets[flagType] = std::bitset<MAX_ENTITIES>();
			flagDefaults[flagType] = flagDefault;
		}

		bool GetComponentFlag(FlagType flagType, Entity entity)
		{
			assert(componentArray.Contains(entity) && "Accessing non-existent component.");

			return flagSets[flagType][entity];
		}

		void SetComponentFlag(FlagType flagType, Entity entity, bool flag)
		{
			assert(componentArray.Contains(entity) && "Accessing non-existent component.");

			flagSets[flagType][entity] = flag;
		}

		void EntityDestroyed(Entity entity) override
		{
			if (componentArray.Contains(entity))
			{
				// Remove entities component if it existed
				RemoveComponent(entity);
			}
		}

	private:

		// Packed array of components
		PackedArray<ECS::Entity, ComponentT, MAX_ENTITIES> componentArray;

		std::unordered_map<FlagType, std::bitset<MAX_ENTITIES>> flagSets;
		std::unordered_map<FlagType, bool> flagDefaults; // What to have each flag type default to
	};

	//////////////////////////////////////////////////
	// Component Manager
	//////////////////////////////////////////////////

	class ComponentManager
	{
	public:

		ComponentManager()
		{
			nextComponentType = 0;
			nextFlagType = 0;
		}

		~ComponentManager()
		{
			componentTypes.clear();
			componentArrays.clear();
			flagTypes.clear();
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
			const char* typeName = typeid(ComponentT).name();

			assert(componentTypes.find(typeName) != componentTypes.end() && "ComponentType not registered before use");

			// Return true if array has component for this entity
			return GetComponentArray<ComponentT>()->HasComponent(entity);
		}

		// Functions for Registering and Updating Component Flags
		template<typename FlagT>
		void RegisterComponentFlag(bool defaultFlag)
		{
			const char* typeName = typeid(FlagT).name();

			assert(flagTypes.find(typeName) == flagTypes.end() && "Registering flag type more than once");
			assert(nextFlagType < MAX_FLAGS && "Registering more than maximum number of flags");

			// Add new flag type to flag type map
			flagTypes.insert({typeName, nextFlagType});
				 
			// Add flag bitset to all component arrays
			for (auto const& pair : componentArrays)
			{
				auto const& componentArray = pair.second;

				componentArray->RegisterComponentFlag(nextFlagType, defaultFlag);
			}

			// Increment next flag type
			nextFlagType++;
		}

		template<typename ComponentT, typename FlagT>
		bool GetComponentFlag(Entity entity)
		{
			const char* typeName = typeid(ComponentT).name();

			assert(componentTypes.find(typeName) != componentTypes.end() && "ComponentType not registered before use");

			const char* flagTypeName = typeid(FlagT).name();

			assert(flagTypes.find(flagTypeName) != flagTypes.end() && "FlagType not registered before use");

			return GetComponentArray<ComponentT>()->GetComponentFlag(flagTypes[flagTypeName], entity);
		}

		template<typename ComponentT, typename FlagT>
		void SetComponentFlag(Entity entity, bool flag)
		{
			const char* typeName = typeid(ComponentT).name();

			assert(componentTypes.find(typeName) != componentTypes.end() && "ComponentType not registered before use");

			const char* flagTypeName = typeid(FlagT).name();

			assert(flagTypes.find(flagTypeName) != flagTypes.end() && "FlagType not registered before use");

			GetComponentArray<ComponentT>()->SetComponentFlag(flagTypes[flagTypeName], entity, flag);
		}

		void EntityDestroyed(Entity entity) const
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

		// ComponentType type to be assigned to next registered component
		ComponentType nextComponentType;

		// Map from type string pointer to component type
		std::unordered_map<const char*, ComponentType> componentTypes;

		// Map from type string pointer to component array
		std::unordered_map<const char*, std::shared_ptr<IComponentArray>> componentArrays;

		// FlagType to be assigned to next registered flag
		FlagType nextFlagType;

		// Map from type string pointer to flag type
		std::unordered_map<const char*, FlagType> flagTypes;

		template<typename ComponentT>
		std::shared_ptr<ComponentArray<ComponentT>> GetComponentArray()
		{
			const char* typeName = typeid(ComponentT).name();

			assert(componentTypes.find(typeName) != componentTypes.end() && "ComponentType not registered before use");

			return std::static_pointer_cast<ComponentArray<ComponentT>>(componentArrays[typeName]);
		}
	};

	//////////////////////////////////////////////////
	// System Manager
	//////////////////////////////////////////////////

	// Stores all signatures used by a system
	typedef std::unordered_map<std::string_view, Signature> SignatureMap;

	class SystemManager
	{
	public:

		SystemManager() {}

		~SystemManager()
		{
			signatureMaps.clear();
			systemsMap.clear();
			systemsVector.clear();
			systemsByUpdateOrderMap.clear();
		}

		template<typename SystemT>
		std::shared_ptr<SystemT> RegisterSystem(std::shared_ptr<World> world)
		{
			const char* typeName = typeid(SystemT).name();

			assert(systemsMap.find(typeName) == systemsMap.end() && "Registering system more than once.");

			// Create New Signature Map for this System
			signatureMaps.insert({ typeName, SignatureMap() });

			// Create and return pointer to system
			std::shared_ptr<SystemT> system = std::make_shared<SystemT>();
			std::shared_ptr<System> systemBase = std::static_pointer_cast<System>(system);

			// Set system world pointer
			systemBase->SetWorld(world);

			// Add System to Vectors/Maps
			systemsVector.push_back(systemBase);
			systemsMap.insert({ typeName, systemBase });
			systemsByUpdateOrderMap.insert({ systemBase->GetInfo().updateOrder, systemBase });

			return system;
		}

		template<typename SystemT>
		void SetSignature(std::string_view signatureName, Signature signature)
		{
			const char* typeName = typeid(SystemT).name();

			assert(systemsMap.find(typeName) != systemsMap.end() && "System used before registered.");

			// Insert New Signature for this System
			signatureMaps.at(typeName).insert({ signatureName, signature });
				
			// Insert New Set for this System
			systemsMap.at(typeName)->entityMap.insert({ signatureName, std::set<Entity>() });
		}

		template<typename SystemT>
		SystemT& GetSystem()
		{
			const char* typeName = typeid(SystemT).name();

			assert(systemsMap.find(typeName) != systemsMap.end() && "System used before registered.");

			return std::static_pointer_cast<SystemT>(systemsMap[typeName]);
		}

		const std::vector<std::shared_ptr<System>>& GetAllSystems()
		{
			return systemsVector;
		}

		void GetSystemsWithUpdateOrder(UpdateOrder updateOrder, std::vector<std::shared_ptr<System>>& outSystems)
		{
			assert(systemsByUpdateOrderMap.find(updateOrder) != systemsByUpdateOrderMap.end() && "Trying to access update order which doesn't have any systems assigned");

			outSystems.clear();

			for (auto it = systemsByUpdateOrderMap.lower_bound(updateOrder); it != systemsByUpdateOrderMap.upper_bound(updateOrder); ++it)
			{
				outSystems.push_back((*it).second);
			}
		}

		void EntityDestroyed(Entity entity)
		{
			// Erase destroyed entity from all system lists
			// Entities is a set so no check needed
			for (auto const& pair : systemsMap)
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
			for (auto const& systemTypePairs : systemsMap)
			{
				auto const& type = systemTypePairs.first;
				auto const& system = systemTypePairs.second;
				auto const& systemSignatureMap = signatureMaps[type];

				// Iterate over each signature of of the system
				for (auto const& systemSignaturePairs : systemSignatureMap)
				{
					auto const& signatureName = systemSignaturePairs.first;
					auto const& systemSignature = systemSignaturePairs.second;

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
		std::unordered_map<const char*, SignatureMap> signatureMaps;

		// Vector of system pointers
		std::vector<std::shared_ptr<System>> systemsVector;

		// Map from system type string to system pointer
		std::unordered_map<const char*, std::shared_ptr<System>> systemsMap;

		// Map from update order to system pointers
		std::multimap<UpdateOrder, std::shared_ptr<System>> systemsByUpdateOrderMap;
	};

	//////////////////////////////////////////////////
	// ECS World
	//////////////////////////////////////////////////

	class World
	{
	public:

		World()
		{
			// Create pointers to each manager
			componentManager = std::make_unique<ComponentManager>();
			entityManager = std::make_unique<EntityManager>();
			systemManager = std::make_unique<SystemManager>();
			eventManager = std::make_unique<EventManager>();

			RegisterEntityFlag<FlagDeleted>();
		}

		~World()
		{
			for (auto entity : entityManager->GetActiveEntities())
			{
				DestroyEntity(entity);
			}

			entityManager = nullptr;
			componentManager = nullptr;
			systemManager = nullptr;
			eventManager = nullptr;
		}

		void Update()
		{
			// Remove all entities marked for deletion
			for (auto entity : entityManager->GetActiveEntities())
			{
				if (GetEntityFlag<FlagDeleted>(entity))
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

		// Entity Methods
		void InitEntitySystem()
		{
			entityManager->Init();
		}

		void InitEntitySystem(std::set<Entity>& entities)
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

		std::string GetEntityName(Entity entity) const
		{
			return entityManager->GetName(entity);
		}

		Signature GetEntitySignature(Entity entity)
		{
			return entityManager->GetSignature(entity);
		}

		template<typename FlagT>
		void RegisterEntityFlag(bool defaultFlag = false)
		{
			entityManager->RegisterEntityFlag<FlagT>(defaultFlag);
		}

		template<typename FlagT>
		bool GetEntityFlag(Entity entity)
		{
			return entityManager->GetEntityFlag<FlagT>(entity);
		}

		template<typename FlagT>
		void SetEntityFlag(Entity entity, bool flag)
		{
			entityManager->SetEntityFlag<FlagT>(entity, flag);
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

		template<typename FlagT>
		void RegisterComponentFlag(bool defaultFlag = false)
		{
			componentManager->RegisterComponentFlag<FlagT>(defaultFlag);
		}

		template<typename ComponentT, typename FlagT>
		bool GetComponentFlag(Entity entity)
		{
			return componentManager->GetComponentFlag<ComponentT, FlagT>(entity);
		}

		template<typename ComponentT, typename FlagT>
		void SetComponentFlag(Entity entity, bool flag)
		{
			componentManager->SetComponentFlag<ComponentT, FlagT>(entity, flag);
		}

		// System Methods

		template<typename SystemT>
		std::shared_ptr<SystemT> RegisterSystem()
		{
			std::shared_ptr<World> worldPtr{ this };
			return systemManager->RegisterSystem<SystemT>(worldPtr);
		}

		template<typename SystemT>
		std::shared_ptr<SystemT> GetSystem()
		{
			return systemManager->GetSystem<SystemT>();
		}

		const std::vector<std::shared_ptr<System>>& GetAllSystems()
		{
			return systemManager->GetAllSystems();
		}

		void GetSystemsWithUpdateOrder(UpdateOrder updateOrder, std::vector<std::shared_ptr<System>>& outSystems)
		{
			systemManager->GetSystemsWithUpdateOrder(updateOrder, outSystems);
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

		// Event Manager Methods

		template<typename EventT>
		void RegisterEvent()
		{
			eventManager->RegisterEvent<EventT>();
		}

		template<typename EventT>
		void PublishEvent(EventT event)
		{
			eventManager->Publish<EventT>(event);
		}

		template<typename EventT>
		void SubscribeToEvent(std::shared_ptr<RingBuffer<EventT>> buffer)
		{
			eventManager->Subscribe(buffer);
		}

	private:

		std::unique_ptr<ComponentManager> componentManager;
		std::unique_ptr<EntityManager> entityManager;
		std::unique_ptr<SystemManager> systemManager;
		std::unique_ptr<EventManager> eventManager;
	};
}