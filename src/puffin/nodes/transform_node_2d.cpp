#include "puffin/nodes/transform_node_2d.h"

#include "puffin/core/engine.h"
#include "puffin/ecs/entt_subsystem.h"
#include "Components/TransformComponent2D.h"

namespace puffin::core
{
	class Engine;
}

namespace puffin
{
	TransformNode2D::TransformNode2D(const std::shared_ptr<core::Engine>& engine, const PuffinID& id) : Node(engine, id)
	{
		m_name = "TransformNode2D";

		add_component<TransformComponent2D>();
	}

	void TransformNode2D::begin_play()
	{
		Node::begin_play();
	}

	void TransformNode2D::update(const double delta_time)
	{

	}

	void TransformNode2D::physics_update(const double delta_time)
	{

	}

	void TransformNode2D::end_play()
	{

	}

	bool TransformNode2D::has_transform_2d() const
	{
		return true;
	}

	const TransformComponent2D& TransformNode2D::transform_2d() const
	{
		return transform();
	}

	TransformComponent2D& TransformNode2D::transform_2d()
	{
		return transform();
	}

	const TransformComponent2D& TransformNode2D::transform() const
	{
		return get_component<TransformComponent2D>();
	}

	TransformComponent2D& TransformNode2D::transform()
	{
		m_transform_changed = true;

		return get_component<TransformComponent2D>();
	}

#ifdef PFN_DOUBLE_PRECISION
	const Vector2d& TransformNode2D::position() const
	{
		return transform().position;
	}

	Vector2d& TransformNode2D::position()
	{
		return transform().position;
	}

	void TransformNode2D::set_position(const Vector2d& position)
	{
		m_registry->patch<TransformComponent2D>(m_entity, [&position](auto& transform) { transform.position = position; });

		m_transform_changed = true;
	}
#else
	const Vector2f& TransformNode2D::position() const
	{
		return get_transform().position;
	}

	Vector2f& TransformNode2D::position()
	{
		return transform().position;
	}

	void TransformNode2D::set_position(const Vector2f& position)
	{
		m_registry->patch<TransformComponent2D>(m_entity, [&position](auto& transform) { transform.position = position; });

		m_transform_changed = true;
	}
#endif

	const float& TransformNode2D::rotation() const
	{
		return transform().rotation;
	}

	float& TransformNode2D::rotation()
	{
		return transform().rotation;
	}

	void TransformNode2D::set_rotation(const float& rotation)
	{
		m_registry->patch<TransformComponent2D>(m_entity, [&rotation](auto& transform) { transform.rotation = rotation; });

		m_transform_changed = true;
	}

	const Vector2f& TransformNode2D::scale() const
	{
		return transform().scale;
	}

	Vector2f& TransformNode2D::scale()
	{
		return transform().scale;
	}

	void TransformNode2D::set_scale(const Vector2f& scale)
	{
		m_registry->patch<TransformComponent2D>(m_entity, [&scale](auto& transform) { transform.scale = scale; });

		m_transform_changed = true;
	}
}
