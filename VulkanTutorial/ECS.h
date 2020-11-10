#pragma once

#include "stdint.h"

#include <queue>
#include <array>
#include <bitset>
#include <set>
#include <unordered_map>
#include <cassert>
#include <memory>

namespace Puffin
{
	namespace ECS
	{
		using Entity = uint32_t;
		const Entity MAX_ENTITIES = 5000; // Maximum number of Entities in Scene

		using ComponentType = uint8_t;
		const ComponentType MAX_COMPONENTS = 32; // Maximum number of uniqe components for each entity

		using Signature = std::bitset<MAX_COMPONENTS>;

		////////////////////////////////////////
		// Entity Manager
		////////////////////////////////////////

		class EntityManager
		{
		public:

			EntityManager();
			~EntityManager();

			Entity CreateEntity();
			void DestroyEntity(Entity entity);

			void SetSignature(Entity entity, Signature signature);
			Signature GetSignature(Entity entity);

		private:

			// Queue of unused Entity IDs
			std::queue<Entity> availableEntities;

			// Array of signatures with index corresponding to entity ID
			std::array<Signature, MAX_ENTITIES> entitySignatures;

			// Number of living Entities
			uint32_t livingEntityCount;
		};

		////////////////////////////////////////
		// Component Array
		////////////////////////////////////////

		class IComponentArray
		{
		public:
			virtual ~IComponentArray() = default;
			virtual void EntityDestroyed(Entity entity) = 0;
		};

		template<typename T>
		class ComponentArray : public IComponentArray
		{
		public:
			void InsertData(Entity entity, T component);
			void RemoveData(Entity entity);
			T& GetData(Entity entity);

			void EntityDestroyed(Entity entity) override;

		private:

			// Packed array of components (of type T) with each spot being uniw to an entity
			std::array<T, MAX_ENTITIES> componentArray;

			// Map from entity ID to array index
			std::unordered_map<Entity, size_t> entityToIndexMap;

			// Map from array index to entity ID
			std::unordered_map<size_t, Entity> indexToEntityMap;

			// Total size of valid entries in array
			size_t entrySize;
		};

		////////////////////////////////////////
		// Component Manager
		////////////////////////////////////////

		class ComponentManager
		{
		public:

			template<typename T>
			void RegisterComponent();

			template<typename T>
			ComponentType GetComponentType();

			template<typename T>
			void AddComponent(Entity entity, T component);

			template<typename T>
			void RemoveComponent(Entity entity);

			template<typename T>
			T& GetComponent(Entity entity);

			void EntityDestroyed(Entity entity);

		private:

			// Map from type string pointer to component type
			std::unordered_map<const char*, ComponentType> componentTypes;

			// Map from type string pointer to a component array
			std::unordered_map<const char*, std::shared_ptr<IComponentArray>> componentArrays;

			// The component type to be assigned to next registered component
			ComponentType nextComponentType;

			// Get statically cast pointer to component array
			template<typename T>
			std::shared_ptr<ComponentArray<T>> GetComponentArray();

		};

		////////////////////////////////////////
		// System
		////////////////////////////////////////

		class System
		{
		public:

			inline void Add(Entity entity)
			{
				entities.insert(entity);
			}

			inline void Remove(Entity entity)
			{
				entities.erase(entity);
			}

		protected:
			std::set<Entity> entities;
		};

		////////////////////////////////////////
		// System Manager
		////////////////////////////////////////

		class SystemManager
		{
		public:

			template<typename T>
			std::shared_ptr<T> RegisterSystem();

			template<typename T>
			void SetSignature(Signature signature);

			void EntityDestroyed(Entity entity);
			void EntitySignatureChanged(Entity entity, Signature entitySignature);

		private:

			// Map from system type string pointer to a signature
			std::unordered_map<const char*, Signature> signatures;

			// Map from system type string pointer to system pointer
			std::unordered_map<const char*, std::shared_ptr<System>> systems;

		};

		////////////////////////////////////////
		// ECS Coordinator
		////////////////////////////////////////

		class Coordinator
		{
		public:

			// Entity Methods
			void Init();
			Entity CreateEntity();
			void DestroyEntity(Entity entity);

			// Component Methods
			template<typename T>
			void RegisterComponent();

			template<typename T>
			void AddComponent(Entity entity, T component);

			template<typename T>
			void RemoveComponent(Entity entity);

			template<typename T>
			T& GetComponent(Entity entity);

			template<typename T>
			ComponentType GetComponentType();

			// System Methods
			template<typename T>
			std::shared_ptr<T> RegisterSystem();

			template<typename T>
			void SetSystemSignature(Signature signature);

		private:

			std::unique_ptr<ComponentManager> componentManager;
			std::unique_ptr<EntityManager> entityManager;
			std::unique_ptr<SystemManager> systemManager;

		};
	}
}