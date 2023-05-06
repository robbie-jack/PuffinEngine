#pragma once

#include "ECS/ECS.h"
#include "ECS/EntityID.h"
#include "ECS/ComponentType.h"

#include <cstdint>
#include <memory>
#include <string>

namespace puffin::ECS
{
	class Entity
	{
	public:

		Entity(std::shared_ptr<World> world, EntityID entityID) : m_world(world), m_id(entityID) {}

		~Entity() { m_world = nullptr; }

		Entity(const Entity& entity)
		{
			m_world = entity.m_world;
			m_id = entity.m_id;
		}

		Entity(Entity&& entity) noexcept
		{
			m_world = entity.m_world;
			m_id = entity.m_id;
		}

		Entity& operator=(const Entity& entity)
		{
			return *this = entity;
		}

		Entity& operator=(Entity&& entity) noexcept
		{
			return *this = entity;
		}

		operator EntityID() const
		{
			return m_id;
		}

		bool operator==(EntityID id) const
		{
			return m_id == id;
		}

		EntityID ID() const { return m_id; }

		std::shared_ptr<ECS::World> World() const { return m_world; }

		////////////////////////////////
		// Entity Methods
		////////////////////////////////

		void SetName(std::string name) const
		{
			m_world->SetEntityName(m_id, name);
		}

		std::string GetName() const
		{
			return m_world->GetEntityName(m_id);
		}

		Signature GetSignature() const
		{
			return m_world->GetEntitySignature(m_id);
		}

		template<typename FlagT>
		void SetFlag(bool flag) const
		{
			m_world->SetEntityFlag<FlagT>(m_id, flag);
		}

		template<typename FlagT>
		bool GetFlag() const
		{
			return m_world->GetEntityFlag<FlagT>(m_id);
		}

		////////////////////////////////
		// Component Methods
		////////////////////////////////

		template<typename CompT>
		void AddComponent() const
		{
			return m_world->AddComponent<CompT>(m_id);
		}

		template<typename CompT>
		CompT& GetComponent() const
		{
			return m_world->GetComponent<CompT>(m_id);
		}

		template<typename CompT>
		CompT& AddAndGetComponent() const
		{
			return m_world->AddAndGetComponent<CompT>(m_id);
		}

		template<typename CompT>
		void RemoveComponent() const
		{
			m_world->RemoveComponent<CompT>(m_id);
		}

		template<typename CompT>
		bool HasComponent() const
		{
			return m_world->HasComponent<CompT>(m_id);
		}

		template<typename CompT, typename FlagT>
		bool GetComponentFlag() const
		{
			return m_world->GetComponentFlag<CompT, FlagT>(m_id);
		}

		template<typename CompT, typename FlagT>
		void SetComponentFlag(bool flag) const
		{
			m_world->SetComponentFlag<CompT, FlagT>(m_id, flag);
		}

	private:

		std::shared_ptr<ECS::World> m_world = nullptr;
		EntityID m_id = INVALID_ENTITY;
	};

	typedef std::shared_ptr<Entity> EntityPtr;

	class EntityCache
	{
		static EntityCache* s_instance;

		EntityCache() = default;

	public:

		static EntityCache* Get()
		{
			if (!s_instance)
				s_instance = new EntityCache();

			return s_instance;
		}

		static void Clear()
		{
			delete s_instance;
			s_instance = nullptr;
		}

		~EntityCache()
		{
			m_entities.clear();
		}

		std::shared_ptr<Entity>& CreateEntity(std::shared_ptr<World> world)
		{
			auto entity = std::make_shared<Entity>(world, world->CreateEntity());

			m_entities[entity->ID()] = entity;

			return m_entities[entity->ID()];
		}

		std::shared_ptr<Entity> GetEntity(std::shared_ptr<ECS::World> world, EntityID id)
		{
			if (!world->EntityExists(id))
			{
				if (m_entities.count(id) == 1)
				{
					m_entities.erase(id);
				}

				return nullptr;
			}

			if (m_entities.count(id) == 0)
			{
				m_entities.emplace(id, std::make_shared<Entity>(world, id));
			}

			return m_entities[id];
		}

		template<typename... ComponentTypes>
		const PackedVector<std::shared_ptr<Entity>>& GetEntities(std::shared_ptr<World> world)
		{
			PackedVector<EntityID> entityIDs;

			Signature signature = world->GetEntities<ComponentTypes ...>(entityIDs);

			std::vector<EntityID> entitiesToRemove;

			// Remove old entities
			for (const auto& entity : m_entityVector[signature])
			{
				const auto& entitySignature = world->GetEntitySignature(entity->ID());

				if ((entitySignature & signature) != signature)
				{
					entitiesToRemove.emplace_back(entity->ID());
				}
			}

			for (const auto& entityID : entitiesToRemove)
			{
				m_entityVector[signature].erase(entityID);
			}

			// Add any new entities
			for (const auto entityID : entityIDs)
			{
				if (!m_entityVector[signature].contains(entityID))
				{
					m_entityVector[signature].insert(entityID, GetEntity(world, entityID));
				}
			}

			return m_entityVector[signature];
		}

	private:

		std::unordered_map<EntityID, std::shared_ptr<Entity>> m_entities; // Cached Entities

		std::unordered_map<Signature, PackedVector<std::shared_ptr<Entity>>> m_entityVector; // Cached Entity lists

	};

	static std::shared_ptr<Entity> CreateEntity(std::shared_ptr<World> world)
	{
		return EntityCache::Get()->CreateEntity(world);
	}

	static std::shared_ptr<Entity> GetEntity(std::shared_ptr<World> world, EntityID id)
	{
		return EntityCache::Get()->GetEntity(world, id);
	}

	template<typename... ComponentTypes>
	static void GetEntities(std::shared_ptr<World> world, PackedVector<std::shared_ptr<Entity>>& entities)
	{
		entities = EntityCache::Get()->GetEntities<ComponentTypes ...>(world);
	}
}
