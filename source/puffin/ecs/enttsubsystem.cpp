#include "puffin/ecs/enttsubsystem.h"

namespace puffin::ecs
{
	EnTTSubsystem::EnTTSubsystem(const std::shared_ptr<core::Engine>& engine): Subsystem(engine)
	{
	}

	EnTTSubsystem::~EnTTSubsystem()
	{
		mEngine = nullptr;
	}

	void EnTTSubsystem::Initialize(core::SubsystemManager* subsystem_manager)
	{
		Subsystem::Initialize(subsystem_manager);

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

		m_id_to_entity.clear();
		m_should_be_serialized.clear();
		m_entity_to_id.clear();
	}

	UUID EnTTSubsystem::add_entity(bool should_be_serialized)
	{
		const auto entity = m_registry->create();
		const auto id = GenerateId();

		m_id_to_entity.emplace(id, entity);
		m_entity_to_id.emplace(entity, id);

		if (should_be_serialized)
			m_should_be_serialized.emplace(id);

		return id;
	}

	entt::entity EnTTSubsystem::add_entity(const UUID id, bool should_be_serialized)
	{
		if (valid(id))
			return m_id_to_entity.at(id);

		const auto entity = m_registry->create();

		m_id_to_entity.emplace(id, entity);
		m_entity_to_id.emplace(entity, id);

		if (should_be_serialized)
			m_should_be_serialized.emplace(id);

		return entity;
	}

	void EnTTSubsystem::remove_entity(const UUID id)
	{
		m_registry->destroy(m_id_to_entity[id]);

		m_id_to_entity.erase(id);
	}

	bool EnTTSubsystem::valid(const UUID id)
	{
		return m_id_to_entity.find(id) != m_id_to_entity.end();
	}

	entt::entity EnTTSubsystem::get_entity(const UUID& id) const
	{
		assert(m_id_to_entity.find(id) != m_id_to_entity.end() && "EnTTSubsystem::get_entity() - No entity with that id exists");

		const entt::entity& entity = m_id_to_entity.at(id);

		return entity;
	}

	UUID EnTTSubsystem::get_id(const entt::entity& entity) const
	{
		if (m_entity_to_id.count(entity) != 0)
			return m_entity_to_id.at(entity);

		return gInvalidID;
	}

	bool EnTTSubsystem::should_be_serialized(const UUID& id) const
	{
		if (m_should_be_serialized.find(id) != m_should_be_serialized.end())
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	std::shared_ptr<entt::registry> EnTTSubsystem::registry()
	{
		return m_registry;
	}
}
