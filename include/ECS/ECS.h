#pragma once

#include "ECS/EntityID.h"
#include "ECS/ComponentType.h"
#include "ECS/System.hpp"
#include "Engine/Subsystem.hpp"
#include "Engine/EventSubsystem.hpp"

#include "Types/PackedArray.h"
#include "Types/ComponentFlags.h"
#include "Types/UUID.h"

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
	constexpr FlagType MAX_FLAGS = 255;

	//////////////////////////////////////////////////
	// Entity Manager
	//////////////////////////////////////////////////

	class EntityManager
	{
	public:

		EntityManager()
		{
			m_nextFlagType = 0;
		}

		~EntityManager()
		{
			Cleanup();
			m_flagTypes.clear();
			m_flagSets.clear();
			m_flagDefaults.clear();
		}

		void Init(std::set<EntityID>& entities)
		{
			if (bInitialized)
			{
				return;
			}

			for (const auto& entityID : entities)
			{
				m_activeEntities.insert(entityID);
				m_entitySignatures.Insert(entityID, Signature());
				m_entityNames.Insert(entityID, "Entity");

				// Set all flags back to default
				for (auto& [fst, snd] : m_flagSets)
				{
					snd[entityID] = m_flagDefaults[fst];
				}
			}

			bInitialized = true;
		}

		void Cleanup()
		{
			m_activeEntities.clear();
			m_entityNames.Clear();
			m_entitySignatures.Clear();

			bInitialized = false;
		}

		EntityID CreateEntity()
		{
			assert(m_activeEntities.size() < MAX_ENTITIES && "Max number of allowed entities reached");

			// Get next available ID from queue
			const EntityID entityID;

			m_activeEntities.insert(entityID);
			m_entitySignatures.Insert(entityID, Signature());
			m_entityNames.Insert(entityID, "Entity");

			// Set all flags back to default
			for (auto& [fst, snd] : m_flagSets)
			{
				snd.Insert(entityID);
				snd[entityID] = m_flagDefaults[fst];
			}

			return entityID;
		}

		void DestroyEntity(EntityID entityID)
		{
			assert(m_activeEntities.count(entityID) == 1 && "Entity doesn't exists");

			// Reset signature for this entity
			m_entitySignatures.Erase(entityID);
			m_entityNames.Erase(entityID);

			m_activeEntities.erase(entityID);

			for (auto& [fst, snd] : m_flagSets)
			{
				snd.Erase(entityID);
			}
		}

		bool EntityExists(EntityID entityID) const
		{
			if (m_activeEntities.count(entityID) == 0)
			{
				return false;
			}

			return true;
		}

		void SetName(EntityID entityID, std::string name)
		{
			assert(m_activeEntities.count(entityID) == 1 && "Entity doesn't exists");

			// Update Entity Name
			m_entityNames[entityID] = name;
		}

		std::string GetName(EntityID entityID)
		{
			assert(m_activeEntities.count(entityID) == 1 && "Entity doesn't exists");

			// Return Entity Name
			return m_entityNames[entityID];
		}

		void SetSignature(EntityID entity, Signature signature)
		{
			assert(m_activeEntities.count(entity) == 1 && "Entity doesn't exists");

			// Update this entity's signature
			m_entitySignatures[entity] = signature;
		}

		const Signature& GetSignature(EntityID entityID)
		{
			assert(m_activeEntities.count(entityID) == 1 && "Entity doesn't exists");

			// Get entity signature
			return m_entitySignatures[entityID];
		}

		template<typename FlagT>
		void RegisterEntityFlag(bool flagDefault)
		{
			const char* typeName = typeid(FlagT).name();

			assert(m_flagTypes.find(typeName) == m_flagTypes.end() && "Registering flag type more than once");
			assert(m_nextFlagType < MAX_FLAGS && "Registering more than maximum number of flags");

			// Add new flag type to flag type map
			m_flagTypes.insert({typeName, m_nextFlagType});

			// Add new flag bitset
			m_flagSets.emplace(m_flagTypes[typeName], PackedBitset<MAX_ENTITIES>());
			m_flagDefaults[m_flagTypes[typeName]] = flagDefault;

			m_nextFlagType++;
		}

		template<typename FlagT>
		bool GetEntityFlag(EntityID entityID)
		{
			const char* flagTypeName = typeid(FlagT).name();			

			assert(m_activeEntities.count(entityID) == 1 && "Entity doesn't exists");
			assert(m_flagTypes.find(flagTypeName) != m_flagTypes.end() && "FlagType not registered before use");

			return m_flagSets[m_flagTypes[flagTypeName]][entityID];
		}

		template<typename FlagT>
		void SetEntityFlag(EntityID entityID, bool flag)
		{
			const char* flagTypeName = typeid(FlagT).name();

			assert(m_activeEntities.count(entityID) == 1 && "Entity doesn't exists");
			assert(m_flagTypes.find(flagTypeName) != m_flagTypes.end() && "FlagType not registered before use");

			m_flagSets[m_flagTypes[flagTypeName]][entityID] = flag;
		}

		std::set<EntityID> GetActiveEntities()
		{
			return m_activeEntities;
		}

		void GetEntities(Signature signature, std::vector<EntityID>& outEntities) const
		{
			outEntities.clear();

			for (auto entityID : m_activeEntities)
			{
				const Signature& entitySignature = m_entitySignatures[entityID];

				if ((entitySignature & signature) == signature)
				{
					outEntities.push_back(entityID);
				}
			}
		}

		// Get count of active entities
		const size_t& GetEntityCount() const
		{
			return m_activeEntities.size();
		}

	private:

		std::set<EntityID> m_activeEntities;

		PackedArray<std::string, MAX_ENTITIES> m_entityNames;
		PackedArray<Signature, MAX_ENTITIES> m_entitySignatures;

		// FlagType to be assigned to next registered flag
		FlagType m_nextFlagType;

		// Map from type string pointer to flag type
		std::unordered_map<const char*, FlagType> m_flagTypes;

		std::unordered_map<FlagType, PackedBitset<MAX_ENTITIES>> m_flagSets;
		std::unordered_map<FlagType, bool> m_flagDefaults; // What to have each flag type default to

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
			m_componentArray.Clear();
			m_flagSets.clear();
			m_flagDefaults.clear();
		}

		void AddComponent(EntityID entity) override
		{
			assert(!m_componentArray.Contains(entity) && "Entity already has a component of this type");

			m_componentArray.Insert(entity, ComponentT());

			// Set all flags back to default
			for (auto& [fst, snd] : m_flagSets)
			{
				snd.Insert(entity, m_flagDefaults[fst]);
			}
		}

		ComponentT& GetComponent(EntityID entity)
		{
			assert(m_componentArray.Contains(entity) && "Retrieving non-existent component.");

			return m_componentArray[entity];
		}

		void RemoveComponent(EntityID entity)
		{
			assert(m_componentArray.Contains(entity) && "Removing non-existent component.");

			m_componentArray.Erase(entity);

			for (auto& [fst, snd] : m_flagSets)
			{
				snd.Erase(entity);
			}
		}

		bool HasComponent(EntityID entity) override
		{
			return m_componentArray.Contains(entity);
		}

		void RegisterComponentFlag(FlagType flagType, bool flagDefault) override
		{
			m_flagSets.emplace(flagType, PackedBitset<MAX_ENTITIES>());
			m_flagDefaults[flagType] = flagDefault;
		}

		bool GetComponentFlag(FlagType flagType, EntityID entity)
		{
			assert(m_componentArray.Contains(entity) && "Accessing non-existent component.");

			return m_flagSets[flagType][entity];
		}

		void SetComponentFlag(FlagType flagType, EntityID entity, bool flag)
		{
			assert(m_componentArray.Contains(entity) && "Accessing non-existent component.");

			m_flagSets[flagType][entity] = flag;
		}

		void EntityDestroyed(EntityID entity) override
		{
			if (m_componentArray.Contains(entity))
			{
				// Remove entities component if it existed
				RemoveComponent(entity);
			}
		}

	private:

		// Packed array of components
		PackedArray<ComponentT, MAX_ENTITIES> m_componentArray;

		std::unordered_map<FlagType, PackedBitset<MAX_ENTITIES>> m_flagSets;
		std::unordered_map<FlagType, bool> m_flagDefaults; // What to have each flag type default to
	};

	//////////////////////////////////////////////////
	// Component Manager
	//////////////////////////////////////////////////

	class ComponentManager
	{
	public:

		ComponentManager()
		{
			m_nextComponentType = 0;
			m_nextFlagType = 0;
		}

		~ComponentManager()
		{
			m_componentTypes.clear();
			m_componentArrays.clear();
			m_flagTypes.clear();
		}

		template<typename ComponentT>
		void RegisterComponent()
		{
			const char* typeName = typeid(ComponentT).name();

			assert(m_componentTypes.find(typeName) == m_componentTypes.end() && "Registering component type more than once");

			// Add new component type to component type map
			m_componentTypes.insert({ typeName, m_nextComponentType });

			// Create ComponentT Array Pointers
			std::shared_ptr<ComponentArray<ComponentT>> array = std::make_shared<ComponentArray<ComponentT>>();

			// Cast ComponentArray pointer to IComponentArray and add to component arrays map
			m_componentArrays.insert({ m_nextComponentType, std::static_pointer_cast<IComponentArray>(array) });

			// Increment next component type
			m_nextComponentType++;
		}

		template<typename ComponentT>
		ComponentType GetComponentType()
		{
			const char* typeName = typeid(ComponentT).name();

			assert(m_componentTypes.find(typeName) != m_componentTypes.end() && "ComponentType not registered before use");

			// Return this components type - used for creating signatures
			return m_componentTypes[typeName];
		}

		template<typename ComponentT>
		void AddComponent(EntityID entity)
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
		}

		template<typename ComponentT>
		ComponentT& GetComponent(EntityID entity)
		{
			// Get reference to component for this entity
			return GetComponentArray<ComponentT>()->GetComponent(entity);
		}

		template<typename ComponentT>
		void RemoveComponent(EntityID entity)
		{
			// Remove component from array for this entity
			GetComponentArray<ComponentT>()->RemoveComponent(entity);
		}

		template<typename ComponentT>
		bool HasComponent(EntityID entity)
		{
			const char* typeName = typeid(ComponentT).name();

			assert(m_componentTypes.find(typeName) != m_componentTypes.end() && "ComponentType not registered before use");

			// Return true if array has component for this entity
			return GetComponentArray<ComponentT>()->HasComponent(entity);
		}

		// Register Component Dependencies for other Components
		template<typename ComponentT, typename... RequiredTypes>
		void AddComponentDependencies()
		{
			const char* typeName = typeid(ComponentT).name();

			assert(m_componentTypes.find(typeName) != m_componentTypes.end() && "ComponentType not registered before use");

			const ComponentType componentType = m_componentTypes[typeName];

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

		template<typename ComponentT>
		const std::set<ComponentType>& GetRequiredComponentTypes()
		{
			ComponentType type = GetComponentType<ComponentT>();

			return m_requiredComponentTypes[type];
		}

		// Functions for Registering and Updating Component Flags
		template<typename FlagT>
		void RegisterComponentFlag(bool defaultFlag)
		{
			const char* typeName = typeid(FlagT).name();

			assert(m_flagTypes.find(typeName) == m_flagTypes.end() && "Registering flag type more than once");
			assert(m_nextFlagType < MAX_FLAGS && "Registering more than maximum number of flags");

			// Add new flag type to flag type map
			m_flagTypes.insert({typeName, m_nextFlagType});
				 
			// Add flag bitset to all component arrays
			for (auto const& pair : m_componentArrays)
			{
				auto const& componentArray = pair.second;

				componentArray->RegisterComponentFlag(m_nextFlagType, defaultFlag);
			}

			// Increment next flag type
			m_nextFlagType++;
		}

		template<typename ComponentT, typename FlagT>
		bool GetComponentFlag(EntityID entity)
		{
			const char* typeName = typeid(ComponentT).name();

			assert(m_componentTypes.find(typeName) != m_componentTypes.end() && "ComponentType not registered before use");

			const char* flagTypeName = typeid(FlagT).name();

			assert(m_flagTypes.find(flagTypeName) != m_flagTypes.end() && "FlagType not registered before use");

			return GetComponentArray<ComponentT>()->GetComponentFlag(m_flagTypes[flagTypeName], entity);
		}

		template<typename ComponentT, typename FlagT>
		void SetComponentFlag(EntityID entity, bool flag)
		{
			const char* typeName = typeid(ComponentT).name();

			assert(m_componentTypes.find(typeName) != m_componentTypes.end() && "ComponentType not registered before use");

			const char* flagTypeName = typeid(FlagT).name();

			assert(m_flagTypes.find(flagTypeName) != m_flagTypes.end() && "FlagType not registered before use");

			GetComponentArray<ComponentT>()->SetComponentFlag(m_flagTypes[flagTypeName], entity, flag);
		}

		void EntityDestroyed(EntityID entity) const
		{
			// Notify each component array that an entity has been destroyed
			// If array has component for this entity, remove it
			for (auto const& pair : m_componentArrays)
			{
				auto const& componentArray = pair.second;

				componentArray->EntityDestroyed(entity);
			}
		}

	private:

		// ComponentType type to be assigned to next registered component
		ComponentType m_nextComponentType;

		// Map from type string pointer to component type
		std::unordered_map<const char*, ComponentType> m_componentTypes;

		// Map from type string pointer to component array
		std::unordered_map<ComponentType, std::shared_ptr<IComponentArray>> m_componentArrays;

		std::unordered_map<ComponentType, std::set<ComponentType>> m_requiredComponentTypes; // Map of required components

		// FlagType to be assigned to next registered flag
		FlagType m_nextFlagType;

		// Map from type string pointer to flag type
		std::unordered_map<const char*, FlagType> m_flagTypes;

		template<typename ComponentT>
		std::shared_ptr<ComponentArray<ComponentT>> GetComponentArray()
		{
			const char* typeName = typeid(ComponentT).name();

			assert(m_componentTypes.find(typeName) != m_componentTypes.end() && "ComponentType not registered before use");

			return std::static_pointer_cast<ComponentArray<ComponentT>>(m_componentArrays[GetComponentType<ComponentT>()]);
		}

		std::shared_ptr<IComponentArray> GetComponentArray(ComponentType compType)
		{
			return m_componentArrays[compType];
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

		SystemManager() = default;

		~SystemManager()
		{
			m_signatureMaps.clear();
			m_systemsMap.clear();
		}

		template<typename SystemT>
		std::shared_ptr<SystemT> RegisterSystem(std::shared_ptr<World> world, std::shared_ptr<Core::Engine> engine)
		{
			const char* typeName = typeid(SystemT).name();

			assert(m_systemsMap.find(typeName) == m_systemsMap.end() && "Registering system more than once.");

			// Create New Signature Map for this System
			m_signatureMaps.insert({ typeName, SignatureMap() });

			// Create and return pointer to system
			std::shared_ptr<SystemT> system = std::make_shared<SystemT>();
			std::shared_ptr<System> systemBase = std::static_pointer_cast<System>(system);

			// Set system world pointer
			systemBase->SetWorld(world);
			systemBase->SetEngine(engine);

			// Add System to Vectors/Maps
			m_systemsMap.insert({ typeName, systemBase });

			return system;
		}

		template<typename SystemT>
		void SetSignature(std::string_view signatureName, Signature signature)
		{
			const char* typeName = typeid(SystemT).name();

			assert(m_systemsMap.find(typeName) != m_systemsMap.end() && "System used before registered.");

			// Insert New Signature for this System
			m_signatureMaps.at(typeName).insert({ signatureName, signature });
				
			// Insert New Set for this System
			m_systemsMap.at(typeName)->entityMap.insert({ signatureName, std::set<EntityID>() });
		}

		template<typename SystemT>
		SystemT& GetSystem()
		{
			const char* typeName = typeid(SystemT).name();

			assert(m_systemsMap.find(typeName) != m_systemsMap.end() && "System used before registered.");

			return std::static_pointer_cast<SystemT>(m_systemsMap[typeName]);
		}

		void EntityDestroyed(EntityID entity)
		{
			// Erase destroyed entity from all system lists
			// Entities is a set so no check needed
			for (auto const& pair : m_systemsMap)
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
			for (auto const& systemTypePairs : m_systemsMap)
			{
				auto const& type = systemTypePairs.first;
				auto const& system = systemTypePairs.second;
				auto const& systemSignatureMap = m_signatureMaps[type];

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
		std::unordered_map<const char*, SignatureMap> m_signatureMaps;

		// Map from system type string to system pointer
		std::unordered_map<const char*, std::shared_ptr<System>> m_systemsMap;
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
		void AddComponent(EntityID entity) const
		{
			m_componentManager->AddComponent<ComponentT>(entity);

			auto signature = m_entityManager->GetSignature(entity);
			signature.set(m_componentManager->GetComponentType<ComponentT>(), true);

			const std::set<ComponentType>& requiredTypes = m_componentManager->GetRequiredComponentTypes<ComponentT>();
			for (const auto& componentType : requiredTypes)
			{
				signature.set(componentType, true);
			}

			m_entityManager->SetSignature(entity, signature);

			m_systemManager->EntitySignatureChanged(entity, signature);
		}

		template<typename ComponentT>
		ComponentT& GetComponent(EntityID entity)
		{
			return m_componentManager->GetComponent<ComponentT>(entity);
		}

		template<typename ComponentT>
		ComponentT& AddAndGetComponent(EntityID entity)
		{
			AddComponent<ComponentT>(entity);
			return GetComponent<ComponentT>(entity);
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
			m_componentManager->AddComponentDependencies<ComponentT, RequiredTypes...>();
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