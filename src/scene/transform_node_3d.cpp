#include "scene/transform_node_3d.h"

#include "Components/TransformComponent3D.h"
#include "ECS/EnTTSubsystem.h"
#include "scene/transform_node_2d.h"

namespace puffin::scene
{
	TransformNode3D::TransformNode3D(const std::shared_ptr<core::Engine>& engine, const PuffinID& id) : Node(engine, id)
	{
		m_name = "Transform3D";

		add_component<TransformComponent3D>();
	}

	void TransformNode3D::begin_play()
	{
		Node::begin_play();
	}

	void TransformNode3D::update(const double delta_time)
	{
		Node::update(delta_time);
	}

	void TransformNode3D::physics_update(const double delta_time)
	{
		Node::physics_update(delta_time);
	}

	void TransformNode3D::end_play()
	{
		Node::end_play();
	}

	const TransformComponent3D& TransformNode3D::get_transform() const
	{
		return get_component<TransformComponent3D>();
	}

#ifdef PFN_USE_DOUBLE_PRECISION
	const Vector3d& TransformNode3D::position() const
	{
		return get_transform().position;
	}

	void TransformNode3D::set_position(const Vector3d& position) const
	{
		m_registry->patch<TransformComponent3D>(m_entity, [&position](auto& transform) { transform.position = position; });
	}
#else
	const Vector3f& TransformNode3D::position() const
	{
		return get_transform().position;
	}

	void TransformNode3D::set_position(const Vector3f& position) const
	{
		m_registry->patch<TransformComponent3D>(m_entity, [&position](auto& transform) { transform.position = position; });
	}
#endif

	const maths::Quat& TransformNode3D::orientation() const
	{
		return get_transform().orientation;
	}

	void TransformNode3D::set_orientation(const maths::Quat& orientation) const
	{
		m_registry->patch<TransformComponent3D>(m_entity, [&orientation](auto& transform) { transform.orientation = orientation; });
	}

	const Vector3f& TransformNode3D::scale() const
	{
		return get_transform().scale;
	}

	void TransformNode3D::set_scale(const Vector3f& scale) const
	{
		m_registry->patch<TransformComponent3D>(m_entity, [&scale](auto& transform) { transform.scale = scale; });
	}
}
