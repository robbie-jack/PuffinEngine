#include "scene/transform_node_3d.h"

#include "Components/TransformComponent3D.h"
#include "ECS/EnTTSubsystem.h"

namespace puffin::scene
{
	void TransformNode3D::create()
	{
		auto entt_subsystem = m_engine->getSystem<ecs::EnTTSubsystem>();

		const auto entity = entt_entity();

		entt_subsystem->registry()->get<SceneObjectComponent>(entity).name = "TransformNode2D";
		auto& transform = entt_subsystem->registry()->emplace<TransformComponent3D>(entity);
	}

	void TransformNode3D::update(double delta_time)
	{

	}

	void TransformNode3D::physics_update(double delta_time)
	{

	}

	void TransformNode3D::destroy()
	{

	}

	TransformComponent3D& TransformNode3D::get_transform()
	{
		auto entt_subsystem = m_engine->getSystem<ecs::EnTTSubsystem>();

		const auto entity = entt_entity();

		return entt_subsystem->registry()->get<TransformComponent3D>(entity);
	}
}
