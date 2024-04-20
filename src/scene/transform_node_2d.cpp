#include "scene/transform_node_2d.h"

#include "Core/Engine.h"
#include "ECS/EnTTSubsystem.h"
#include "Components/TransformComponent2D.h"

namespace puffin::core
{
	class Engine;
}

namespace puffin::scene
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

	const TransformComponent2D& TransformNode2D::get_transform() const
	{
		return get_component<TransformComponent2D>();
	}

#ifdef PFN_DOUBLE_PRECISION
	const Vector2d& TransformNode2D::position() const
	{
		return get_transform().position;
	}

	void TransformNode2D::set_position(const Vector2d& position) const
	{
		m_registry->patch<TransformComponent2D>(m_entity, [&position](auto& transform) { transform.position = position; });
	}
#else
	const Vector2f& TransformNode2D::position() const
	{
		return get_transform().position;
	}

	void TransformNode2D::set_position(const Vector2f& position) const
	{
		m_registry->patch<TransformComponent2D>(m_entity, [&position](auto& transform) { transform.position = position; });
	}
#endif

	const float& TransformNode2D::rotation() const
	{
		return get_transform().rotation;
	}

	void TransformNode2D::set_rotation(const float& rotation) const
	{
		m_registry->patch<TransformComponent2D>(m_entity, [&rotation](auto& transform) { transform.rotation = rotation; });
	}

	const Vector2f& TransformNode2D::scale() const
	{
		return get_transform().scale;
	}

	void TransformNode2D::set_scale(const Vector2f& scale) const
	{
		m_registry->patch<TransformComponent2D>(m_entity, [&scale](auto& transform) { transform.scale = scale; });
	}
}
