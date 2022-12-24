#pragma once

#include "ECS/ECS.h"
#include "ECS/EntityID.h"
#include "ECS/ComponentType.h"

#include <cstdint>
#include <memory>
#include <string>

namespace Puffin::ECS
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

	static inline std::shared_ptr<Entity> CreateEntity(std::shared_ptr<World> world)
	{
		return std::make_shared<Entity>(world, world->CreateEntity());
	}

	static inline std::shared_ptr<Entity> GetEntity(std::shared_ptr<World> world, EntityID id)
	{
		if (!world->EntityExists(id))
		{
			return nullptr;
		}

		return std::make_shared<Entity>(world, id);
	}

	template<typename... ComponentTypes>
	static inline void GetEntities(std::shared_ptr<World> world, std::vector<std::shared_ptr<Entity>>& outEntities)
	{
		std::vector<EntityID> entityIDs;

		world->GetEntities<ComponentTypes ...>(entityIDs);

		outEntities.clear();
		outEntities.reserve(entityIDs.size());

		for (const auto entityID : entityIDs)
		{
			outEntities.push_back(GetEntity(world, entityID));
		}
	}
}
