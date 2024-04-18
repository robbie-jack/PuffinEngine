#include "scene/transform_node_3d.h"

#include "Components/TransformComponent3D.h"
#include "ECS/EnTTSubsystem.h"
#include "scene/transform_node_2d.h"

namespace puffin::scene
{
	TransformNode3D::TransformNode3D(const PuffinID& id) : Node(id)
	{
		m_name = "TransformNode3D";

		add_component<TransformComponent3D>();
	}

	void TransformNode3D::begin_play()
	{

	}

	void TransformNode3D::update(double delta_time)
	{

	}

	void TransformNode3D::physics_update(double delta_time)
	{

	}

	void TransformNode3D::end_play()
	{

	}

	TransformComponent3D& TransformNode3D::get_transform()
	{
		return get_component<TransformComponent3D>();
	}
}
