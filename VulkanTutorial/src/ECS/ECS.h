#pragma once

#include "ECS/EntityID.h"
#include "ECS/ComponentType.h"
#include "ECS/System.h"
#include "Engine/Subsystem.hpp"
#include "Engine/EventSubsystem.h"

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

			for (EntityID entity = 1; entity < MAX_ENTITIES; entity++)
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

		void Init(std::set<EntityID>& entities)
		{
			if (bInitialized)
			{
				return;
			}

			for (EntityID entity = 1; entity < MAX_ENTITIES; entity++)
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

		EntityID CreateEntity()
		{
			assert(activeEntityCount < MAX_ENTITIES && "Max number of allowed entities reached");

			// Get next available ID from queue
			EntityID entity = availableEntities.front();
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

		void DestroyEntity(EntityID entity)
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

		bool EntityExists(EntityID entity) const
		{
			assert(entity < MAX_ENTITIES && "Entity out of range");

			if (activeEntities.count(entity) == 0)
			{
				return false;
			}

			return true;
		}

		void SetName(EntityID entity, std::string name)
		{
			assert(entity < MAX_ENTITIES && "Entity out of range");

			// Update Entity Name
			entityNames[entity] = name;
		}

		std::string GetName(EntityID entity)
		{
			assert(entity < MAX_ENTITIES && "Entity out of range");

			// Return Entity Name
			return entityNames[entity];
		}

		void SetSignature(EntityID entity, Signature signature)
		{
			assert(entity < MAX_ENTITIES && "Entity out of range");

			// Update this entity's signature
			entitySignatures[entity] = signature;
		}

		Signature GetSignature(EntityID entity)
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
		bool GetEntityFlag(EntityID entity)
		{
			const char* flagTypeName = typeid(FlagT).name();			

			assert(activeEntities.count(entity) == 1 && "Entity does not exist");
			assert(flagTypes.find(flagTypeName) != flagTypes.end() && "FlagType not registered before use");

			return flagSets[flagTypes[flagTypeName]][entity];
		}

		template<typename FlagT>
		void SetEntityFlag(EntityID entity, bool flag)
		{
			const char* flagTypeName = typeid(FlagT).name();

			assert(activeEntities.count(entity) == 1 && "Entity does not exist");
			assert(flagTypes.find(flagTypeName) != flagTypes.end() && "FlagType not registered before use");

			flagSets[flagTypes[flagTypeName]][entity] = flag;
		}

		std::set<EntityID> GetActiveEntities()
		{
			return activeEntities;
		}

		void GetEntities(Signature signature, std::vector<EntityID>& outEntities) const
		{
			outEntities.clear();

			for (auto entity : activeEntities)
			{
				Signature entitySignature = entitySignatures[entity];

				if ((entitySignature & signature) == signature)
				{
					outEntities.push_back(entity);
				}
			}
		}

		// Get count of active entities
		int GetEntityCount() const
		{
			return activeEntityCount;
		}

	private:

		std::queue<EntityID> availableEntities;
		std::set<EntityID> activeEntities;
		std::unordered_map<int, EntityID> m_indexToEntityMap;

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
		virtual void AddComponent(EntityID entity) = 0;
		virtual bool HasComponent(EntityID entity) = 0;
		virtual void RegisterComponentFlag(FlagType flagType, bool flagDefault) = 0;
		virtual void EntityDestroyed(EntityID entity) = 0;
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

		//ComponentT& AddComponent(EntityID entity)
		//{
		//	assert(!componentArray.Contains(entity) && "Entity already has a component of this type");

		//	componentArray.Insert(entity, ComponentT());

		//	// Set all flags back to default
		//	for (auto& pair : flagSets)
		//	{
		//		pair.second[entity] = flagDefaults[pair.first];
		//	}

		//	return componentArray[entity];
		//}

		void AddComponent(EntityID entity, ComponentT& component)
		{
			assert(!componentArray.Contains(entity) && "Entity already has a component of this type");

			componentArray.Insert(entity, component);

			// Set all flags back to default
			for (auto& pair : flagSets)
			{
				pair.second[entity] = flagDefaults[pair.first];
			}
		}

		void AddComponent(EntityID entity) override
		{
			assert(!componentArray.Contains(entity) && "Entity already has a component of this type");

			componentArray.Insert(entity, ComponentT());

			// Set all flags back to default
			for (auto& pair : flagSets)
			{
				pair.second[entity] = flagDefaults[pair.first];
			}
		}

		void RemoveComponent(EntityID entity)
		{
			assert(componentArray.Contains(entity) && "Removing non-existent component.");

			componentArray.Erase(entity);
		}

		ComponentT& GetComponent(EntityID entity)
		{
			assert(componentArray.Contains(entity) && "Retrieving non-existent component.");

			return componentArray[entity];
		}

		bool HasComponent(EntityID entity) override
		{
			return componentArray.Contains(entity);
		}

		void RegisterComponentFlag(FlagType flagType, bool flagDefault) override
		{
			flagSets[flagType] = std::bitset<MAX_ENTITIES>();
			flagDefaults[flagType] = flagDefault;
		}

		bool GetComponentFlag(FlagType flagType, EntityID entity)
		{
			assert(componentArray.Contains(entity) && "Accessing non-existent component.");

			return flagSets[flagType][entity];
		}

		void SetComponentFlag(FlagType flagType, EntityID entity, bool flag)
		{
			assert(componentArray.Contains(entity) && "Accessing non-existent component.");

			flagSets[flagType][entity] = flag;
		}

		void EntityDestroyed(EntityID entity) override
		{
			if (componentArray.Contains(entity))
			{
				// Remove entities component if it existed
				RemoveComponent(entity);
			}
		}

	private:

		// Packed array of components
		PackedArray<ECS::EntityID, ComponentT, MAX_ENTITIES> componentArray;

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
			componentArrays.insert({ nextComponentType, std::static_pointer_cast<IComponentArray>(array) });

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
		ComponentT& AddComponent(EntityID entity)
		{
			// Add a component to array for this entity
			GetComponentArray<ComponentT>()->AddComponent(entity);

			for (auto& compType : m_requiredComponentTypes[GetComponentType<ComponentT>()])
			{
				const std::shared_ptr<IComponentArray> compArray = GetComponentArray(compType);

				if (!compArray->HasComponent(entity))
				{
					compArray->AddComponent(entity);
				}
			}

			return GetComponentArray<ComponentT>()->GetComponent(entity);
		}

		template<typename ComponentT>
		void AddComponent(EntityID entity, ComponentT& component)
		{
			// Add a component to array for this entity
			GetComponentArray<ComponentT>()->AddComponent(entity, component);

			for (auto& compType : m_requiredComponentTypes[GetComponentType<ComponentT>()])
			{
				const std::shared_ptr<IComponentArray> compArray = GetComponentArray(compType);

				if (!compArray->HasComponent(entity))
				{
					compArray->AddComponent(entity);
				}
			}
		}

		template<typename ComponentT>
		void RemoveComponent(EntityID entity)
		{
			// Remove component from array for this entity
			GetComponentArray<ComponentT>()->RemoveComponent(entity);
		}

		template<typename ComponentT>
		ComponentT& GetComponent(EntityID entity)
		{
			// Get reference to component for this entity
			return GetComponentArray<ComponentT>()->GetComponent(entity);
		}

		template<typename ComponentT>
		bool HasComponent(EntityID entity)
		{
			const char* typeName = typeid(ComponentT).name();

			assert(componentTypes.find(typeName) != componentTypes.end() && "ComponentType not registered before use");

			// Return true if array has component for this entity
			return GetComponentArray<ComponentT>()->HasComponent(entity);
		}

		// Register Component Dependencies for other Components
		template<typename ComponentT, typename... RequiredTypes>
		void AddComponentDependencies()
		{
			const char* typeName = typeid(ComponentT).name();

			assert(componentTypes.find(typeName) != componentTypes.end() && "ComponentType not registered before use");

			const ComponentType componentType = componentTypes[typeName];

			if (sizeof...(RequiredTypes) != 0)
			{
				//Unpack component types into initializer list
				ComponentType requiredTypes[] = { GetComponentType<RequiredTypes>() ... };

				// Iterate over component types, setting bit for each in signature
				for (int i = 0; i < sizeof...(RequiredTypes); i++)
				{
					m_requiredComponentTypes[componentType].insert(requiredTypes[i]);
				}
			}
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
		bool GetComponentFlag(EntityID entity)
		{
			const char* typeName = typeid(ComponentT).name();

			assert(componentTypes.find(typeName) != componentTypes.end() && "ComponentType not registered before use");

			const char* flagTypeName = typeid(FlagT).name();

			assert(flagTypes.find(flagTypeName) != flagTypes.end() && "FlagType not registered before use");

			return GetComponentArray<ComponentT>()->GetComponentFlag(flagTypes[flagTypeName], entity);
		}

		template<typename ComponentT, typename FlagT>
		void SetComponentFlag(EntityID entity, bool flag)
		{
			const char* typeName = typeid(ComponentT).name();

			assert(componentTypes.find(typeName) != componentTypes.end() && "ComponentType not registered before use");

			const char* flagTypeName = typeid(FlagT).name();

			assert(flagTypes.find(flagTypeName) != flagTypes.end() && "FlagType not registered before use");

			GetComponentArray<ComponentT>()->SetComponentFlag(flagTypes[flagTypeName], entity, flag);
		}

		void EntityDestroyed(EntityID entity) const
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
		std::unordered_map<ComponentType, std::shared_ptr<IComponentArray>> componentArrays;

		std::unordered_map<ComponentType, std::set<ComponentType>> m_requiredComponentTypes; // Map of required components

		// FlagType to be assigned to next registered flag
		FlagType nextFlagType;

		// Map from type string pointer to flag type
		std::unordered_map<const char*, FlagType> flagTypes;

		template<typename ComponentT>
		std::shared_ptr<ComponentArray<ComponentT>> GetComponentArray()
		{
			const char* typeName = typeid(ComponentT).name();

			assert(componentTypes.find(typeName) != componentTypes.end() && "ComponentType not registered before use");

			return std::static_pointer_cast<ComponentArray<ComponentT>>(componentArrays[GetComponentType<ComponentT>()]);
		}

		std::shared_ptr<IComponentArray> GetComponentArray(ComponentType compType)
		{
			return componentArrays[compType];
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
		}

		template<typename SystemT>
		std::shared_ptr<SystemT> RegisterSystem(std::shared_ptr<World> world, std::shared_ptr<Core::Engine> engine)
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
			systemBase->SetEngine(engine);

			// Add System to Vectors/Maps
			systemsMap.insert({ typeName, systemBase });

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
			systemsMap.at(typeName)->entityMap.insert({ signatureName, std::set<EntityID>() });
		}

		template<typename SystemT>
		SystemT& GetSystem()
		{
			const char* typeName = typeid(SystemT).name();

			assert(systemsMap.find(typeName) != systemsMap.end() && "System used before registered.");

			return std::static_pointer_cast<SystemT>(systemsMap[typeName]);
		}

		void EntityDestroyed(EntityID entity)
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

		void EntitySignatureChanged(EntityID entity, Signature entitySignature)
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

		// Map from system type string to system pointer
		std::unordered_map<const char*, std::shared_ptr<System>> systemsMap;
	};

	//////////////////////////////////////////////////
	// ECS World
	//////////////////////////////////////////////////

	class World : public Core::Subsystem, public std::enable_shared_from_this<World>
	{
	public:

		World()
		{
			m_shouldUpdate = true;

			// Create pointers to each manager
			m_componentManager = std::make_unique<ComponentManager>();
			m_entityManager = std::make_unique<EntityManager>();
			m_systemManager = std::make_unique<SystemManager>();

			RegisterEntityFlag<FlagDeleted>();
		}

		~World() override = default;

		void Init() override {}

		void Update() override
		{
			// Remove all entities marked for deletion
			for (auto entity : m_entityManager->GetActiveEntities())
			{
				if (GetEntityFlag<FlagDeleted>(entity))
				{
					DestroyEntity(entity);
				}
			}
		}

		void Destroy() override
		{
			for (auto entity : m_entityManager->GetActiveEntities())
			{
				DestroyEntity(entity);
			}

			m_entityManager = nullptr;
			m_componentManager = nullptr;
			m_systemManager = nullptr;
		}

		// Reset Entities and Components, Leave Systems Intact
		void Reset()
		{
			for (auto entity : m_entityManager->GetActiveEntities())
			{
				DestroyEntity(entity);
			}

			m_entityManager->Cleanup();
		}

		////////////////////////////////
		// Entity Methods
		////////////////////////////////

		void InitEntitySystem() const
		{
			m_entityManager->Init();
		}

		void InitEntitySystem(std::set<EntityID>& entities) const
		{
			m_entityManager->Init(entities);
		}

		EntityID CreateEntity() const
		{
			return m_entityManager->CreateEntity();
		}

		template<typename... ComponentTypes>
		void GetEntities(std::vector<EntityID>& outEntities) const
		{
			if (sizeof...(ComponentTypes) != 0)
			{
				//Unpack component types into initializer list
				ComponentType componentTypes[] = { GetComponentType<ComponentTypes>() ... };
				Signature signature;

				// Iterate over component types, setting bit for each in signature
				for (int i = 0; i < sizeof...(ComponentTypes); i++)
				{
					signature.set(componentTypes[i]);
				}

				outEntities.clear();
				m_entityManager->GetEntities(signature, outEntities);
			}
		}

		std::set<EntityID> GetActiveEntities() const
		{
			return m_entityManager->GetActiveEntities();
		}

		void DestroyEntity(EntityID entity) const
		{
			m_entityManager->DestroyEntity(entity);

			m_componentManager->EntityDestroyed(entity);

			m_systemManager->EntityDestroyed(entity);
		}

		bool EntityExists(EntityID entity) const
		{
			return m_entityManager->EntityExists(entity);
		}

		void SetEntityName(EntityID entity, std::string name) const
		{
			m_entityManager->SetName(entity, name);
		}

		std::string GetEntityName(EntityID entity) const
		{
			return m_entityManager->GetName(entity);
		}

		Signature GetEntitySignature(EntityID entity) const
		{
			return m_entityManager->GetSignature(entity);
		}

		template<typename FlagT>
		void RegisterEntityFlag(bool defaultFlag = false) const
		{
			m_entityManager->RegisterEntityFlag<FlagT>(defaultFlag);
		}

		template<typename FlagT>
		bool GetEntityFlag(EntityID entity) const
		{
			return m_entityManager->GetEntityFlag<FlagT>(entity);
		}

		template<typename FlagT>
		void SetEntityFlag(EntityID entity, bool flag) const
		{
			m_entityManager->SetEntityFlag<FlagT>(entity, flag);
		}

		////////////////////////////////
		// Component Methods
		////////////////////////////////
		
		template<typename ComponentT>
		void RegisterComponent() const
		{
			m_componentManager->RegisterComponent<ComponentT>();
		}

		template<typename ComponentT>
		ComponentT& AddComponent(EntityID entity)
		{
			ComponentT& comp = m_componentManager->AddComponent<ComponentT>(entity);

			auto signature = m_entityManager->GetSignature(entity);
			signature.set(m_componentManager->GetComponentType<ComponentT>(), true);
			m_entityManager->SetSignature(entity, signature);

			m_systemManager->EntitySignatureChanged(entity, signature);

			return comp;
		}

		template<typename ComponentT>
		void AddComponent(EntityID entity, ComponentT& component)
		{
			m_componentManager->AddComponent<ComponentT>(entity, component);

			auto signature = m_entityManager->GetSignature(entity);
			signature.set(m_componentManager->GetComponentType<ComponentT>(), true);
			m_entityManager->SetSignature(entity, signature);

			m_systemManager->EntitySignatureChanged(entity, signature);
		}

		template<typename ComponentT>
		void RemoveComponent(EntityID entity) const
		{
			m_componentManager->RemoveComponent<ComponentT>(entity);

			auto signature = m_entityManager->GetSignature(entity);
			signature.set(m_componentManager->GetComponentType<ComponentT>(), false);
			m_entityManager->SetSignature(entity, signature);

			m_systemManager->EntitySignatureChanged(entity, signature);
		}

		template<typename ComponentT>
		ComponentT& GetComponent(EntityID entity)
		{
			return m_componentManager->GetComponent<ComponentT>(entity);
		}

		template<typename ComponentT>
		ComponentType GetComponentType() const
		{
			return m_componentManager->GetComponentType<ComponentT>();
		}

		template<typename ComponentT>
		bool HasComponent(EntityID entity) const
		{
			return m_componentManager->HasComponent<ComponentT>(entity);
		}

		// Register Component Dependencies for other Components
		template<typename ComponentT, typename... RequiredTypes>
		void AddComponentDependencies() const
		{
			m_componentManager->AddComponentDependencies<ComponentT, RequiredTypes>();
		}

		template<typename FlagT>
		void RegisterComponentFlag(bool defaultFlag = false) const
		{
			m_componentManager->RegisterComponentFlag<FlagT>(defaultFlag);
		}

		template<typename ComponentT, typename FlagT>
		bool GetComponentFlag(EntityID entity) const
		{
			return m_componentManager->GetComponentFlag<ComponentT, FlagT>(entity);
		}

		template<typename ComponentT, typename FlagT>
		void SetComponentFlag(EntityID entity, bool flag) const
		{
			m_componentManager->SetComponentFlag<ComponentT, FlagT>(entity, flag);
		}

		////////////////////////////////
		// System Methods
		////////////////////////////////

		template<typename SystemT>
		std::shared_ptr<SystemT> RegisterSystem()
		{
			return m_systemManager->RegisterSystem<SystemT>(shared_from_this(), m_engine);
		}

		template<typename SystemT>
		std::shared_ptr<SystemT> GetSystem()
		{
			return m_systemManager->GetSystem<SystemT>();
		}

		template<typename SystemT>
		void SetSystemSignature(std::string_view signatureName, Signature signature) const
		{
			m_systemManager->SetSignature<SystemT>(signatureName, signature);

			// Update System's local entity list with any new entities
			for (EntityID entity : m_entityManager->GetActiveEntities())
			{
				m_systemManager->EntitySignatureChanged(entity, m_entityManager->GetSignature(entity));
			}
		}

	private:

		std::unique_ptr<ComponentManager> m_componentManager = nullptr;
		std::unique_ptr<EntityManager> m_entityManager = nullptr;
		std::unique_ptr<SystemManager> m_systemManager = nullptr;
	};
}