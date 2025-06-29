#include "ecs/entt_subsystem.h"

namespace puffin::ecs
{
	EnTTSubsystem::EnTTSubsystem(const std::shared_ptr<core::Engine>& engine) : EngineSubsystem(engine)
	{
	}

	EnTTSubsystem::~EnTTSubsystem()
	{
		m_engine = nullptr;
	}

	void EnTTSubsystem::Initialize()
	{
		Subsystem::Initialize();

		m_registry = std::make_shared<entt::registry>();
	}

	void EnTTSubsystem::Deinitialize()
	{
		Subsystem::Deinitialize();

		m_registry = nullptr;
	}

	void EnTTSubsystem::EndPlay()
	{
		m_registry->clear();

		m_idToEntity.clear();
		m_shouldBeSerialized.clear();
		m_entityToId.clear();
	}

	std::string_view EnTTSubsystem::GetName() const
	{
		return reflection::GetTypeString<EnTTSubsystem>();
	}

	UUID EnTTSubsystem::AddEntity(bool shouldBeSerialized)
	{
		const auto entity = m_registry->create();
		const auto id = GenerateId();

		m_idToEntity.emplace(id, entity);
		m_entityToId.emplace(entity, id);

		if (shouldBeSerialized)
			m_shouldBeSerialized.emplace(id);

		return id;
	}

	entt::entity EnTTSubsystem::AddEntity(UUID id, bool shouldBeSerialized)
	{
		if (IsEntityValid(id))
			return m_idToEntity.at(id);

		const auto entity = m_registry->create();

		m_idToEntity.emplace(id, entity);
		m_entityToId.emplace(entity, id);

		if (shouldBeSerialized)
			m_shouldBeSerialized.emplace(id);

		return entity;
	}

	void EnTTSubsystem::RemoveEntity(UUID id)
	{
		m_registry->destroy(m_idToEntity[id]);

		m_idToEntity.erase(id);
	}

	bool EnTTSubsystem::IsEntityValid(const UUID id) const
	{
		return m_idToEntity.find(id) != m_idToEntity.end();
	}

	entt::entity EnTTSubsystem::GetEntity(UUID id) const
	{
		assert(m_idToEntity.find(id) != m_idToEntity.end() && "EnTTSubsystem::GetEntity() - No entity with that id exists");

		const entt::entity& entity = m_idToEntity.at(id);

		return entity;
	}

	UUID EnTTSubsystem::GetID(entt::entity entity) const
	{
		if (m_entityToId.count(entity) != 0)
			return m_entityToId.at(entity);

		return gInvalidID;
	}

	bool EnTTSubsystem::ShouldEntityBeSerialized(const UUID& id) const
	{
		if (m_shouldBeSerialized.find(id) != m_shouldBeSerialized.end())
		{
			return true;
		}

		return false;
	}

	std::shared_ptr<entt::registry> EnTTSubsystem::GetRegistry()
	{
		return m_registry;
	}
}
