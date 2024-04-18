#include "scene/transform_node_2d.h"

#include "Core/Engine.h"
#include "ECS/EnTTSubsystem.h"
#include "Components/TransformComponent2D.h"

namespace puffin::scene
{
	void TransformNode2D::create()
	{
		auto entt_subsystem = m_engine->getSystem<ecs::EnTTSubsystem>();

		const auto entity = entt_entity();

		entt_subsystem->registry()->get<SceneObjectComponent>(entity).name = "TransformNode2D";
		auto& transform = entt_subsystem->registry()->emplace<TransformComponent2D>(entity);
	}

	void TransformNode2D::update(double delta_time)
	{

	}

	void TransformNode2D::physics_update(double delta_time)
	{

	}

	void TransformNode2D::destroy()
	{
		auto entt_subsystem = m_engine->getSystem<ecs::EnTTSubsystem>();

		entt_subsystem->destroyEntity(m_entity_id);
	}

	TransformComponent2D& TransformNode2D::get_transform()
	{
		auto entt_subsystem = m_engine->getSystem<ecs::EnTTSubsystem>();

		const auto entity = entt_entity();

		return entt_subsystem->registry()->get<TransformComponent2D>(entity);
	}
}
