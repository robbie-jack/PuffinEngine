#include "puffin/nodes/transform_node_3d.h"

#include "puffin/components/transform_component_3d.h"
#include "puffin/ecs/entt_subsystem.h"

namespace puffin
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

	void TransformNode3D::update_fixed(const double delta_time)
	{
		Node::update_fixed(delta_time);
	}

	void TransformNode3D::end_play()
	{
		Node::end_play();
	}

	bool TransformNode3D::has_transform_3d() const
	{
		return true;
	}

	const TransformComponent3D* TransformNode3D::transform_3d() const
	{
		return &get_component<TransformComponent3D>();
	}

	TransformComponent3D* TransformNode3D::transform_3d()
	{
		m_transform_changed = true;

		return &get_component<TransformComponent3D>();
	}

#ifdef PFN_DOUBLE_PRECISION
	const Vector3d& TransformNode3D::position() const
	{
		return transform_3d()->position;
	}

	Vector3d& TransformNode3D::position()
	{
		return transform_3d()->position;
	}

	void TransformNode3D::set_position(const Vector3d& position)
	{
		m_registry->patch<TransformComponent3D>(m_entity, [&position](auto& transform) { transform.position = position; });

		m_transform_changed = true;
	}
#else
	const Vector3f& TransformNode3D::position() const
	{
		return transform_3d()->position;
	}

	Vector3f& TransformNode3D::position()
	{
		return transform_3d()->position;
	}

	void TransformNode3D::set_position(const Vector3f& position)
	{
		m_registry->patch<TransformComponent3D>(m_entity, [&position](auto& transform) { transform.position = position; });

		m_transform_changed = true;
	}
#endif

	const maths::Quat& TransformNode3D::orientation() const
	{
		return transform_3d()->orientation_quat;
	}

	maths::Quat& TransformNode3D::orientation()
	{
		return transform_3d()->orientation_quat;
	}

	void TransformNode3D::set_orientation(const maths::Quat& orientation)
	{
		m_registry->patch<TransformComponent3D>(m_entity, [&orientation](auto& transform) { transform.orientation_quat = orientation; });

		m_transform_changed = true;
	}

	const maths::EulerAngles& TransformNode3D::euler_angles() const
	{
		return transform_3d()->orientation_euler_angles;
	}

	maths::EulerAngles& TransformNode3D::euler_angles()
	{
		return transform_3d()->orientation_euler_angles;
	}

	void TransformNode3D::set_euler_angles(const maths::EulerAngles& euler_angles)
	{
		m_registry->patch<TransformComponent3D>(m_entity, [&euler_angles](auto& transform) { transform.orientation_euler_angles = euler_angles; });

		m_transform_changed = true;
	}

	const Vector3f& TransformNode3D::scale() const
	{
		return transform_3d()->scale;
	}

	Vector3f& TransformNode3D::scale()
	{
		return transform_3d()->scale;
	}

	void TransformNode3D::set_scale(const Vector3f& scale)
	{
		m_registry->patch<TransformComponent3D>(m_entity, [&scale](auto& transform) { transform.scale = scale; });

		m_transform_changed = true;
	}
}
