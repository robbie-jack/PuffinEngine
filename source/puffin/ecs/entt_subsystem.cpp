#include "puffin/ecs/entt_subsystem.h"

namespace puffin::ecs
{
	EnTTSubsystem::EnTTSubsystem(const std::shared_ptr<core::Engine>& engine): Subsystem(engine)
	{
	}

	EnTTSubsystem::~EnTTSubsystem()
	{
		m_engine = nullptr;
	}

	void EnTTSubsystem::initialize(core::SubsystemManager* subsystem_manager)
	{
		Subsystem::initialize(subsystem_manager);

		m_registry = std::make_shared<entt::registry>();
	}

	void EnTTSubsystem::deinitialize()
	{
		Subsystem::deinitialize();

		m_registry = nullptr;
	}

	void EnTTSubsystem::end_play()
	{
		m_registry->clear();

		m_id_to_entity.clear();
		m_should_be_serialized.clear();
		m_entity_to_id.clear();
	}

	entt::entity EnTTSubsystem::add_entity(const PuffinID id, bool should_be_serialized)
	{
		if (valid(id))
			return m_id_to_entity.at(id);

		const auto entity = m_registry->create();

		m_id_to_entity.emplace(id, entity);
		m_should_be_serialized.emplace(id, should_be_serialized);
		m_entity_to_id.emplace(entity, id);

		return entity;
	}

	void EnTTSubsystem::remove_entity(const PuffinID id)
	{
		m_registry->destroy(m_id_to_entity[id]);

		m_id_to_entity.erase(id);
	}

	bool EnTTSubsystem::valid(const PuffinID id)
	{
		return m_id_to_entity.find(id) != m_id_to_entity.end();
	}

	entt::entity EnTTSubsystem::get_entity(const PuffinID& id) const
	{
		const entt::entity& entity = m_id_to_entity.at(id);

		return entity;
	}

	bool EnTTSubsystem::should_be_serialized(const PuffinID& id) const
	{
		return m_should_be_serialized.at(id);
	}

	PuffinID EnTTSubsystem::get_id(const entt::entity& entity) const
	{
		if (m_entity_to_id.count(entity) != 0)
			return m_entity_to_id.at(entity);

		return gInvalidID;
	}

	std::shared_ptr<entt::registry> EnTTSubsystem::registry()
	{
		return m_registry;
	}
}
