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

	TransformComponent3D& TransformNode3D::get_transform()
	{
		return get_component<TransformComponent3D>();
	}

	Vector3f& TransformNode3D::position()
	{
		return get_transform().position;
	}

	maths::Quat& TransformNode3D::orientation()
	{
		return get_transform().orientation;
	}

	Vector3f& TransformNode3D::scale()
	{
		return get_transform().scale;
	}
}
