#include "nodes/physics/rigidbody_node_3d.h"

#include "Components/Physics/3D/RigidbodyComponent3D.h"

namespace puffin::physics
{
	RigidbodyNode3D::RigidbodyNode3D(const std::shared_ptr<core::Engine>& engine, const PuffinID& id)
		: TransformNode3D(engine, id)
	{
		m_name = "Rigidbody3D";

		add_component<RigidbodyComponent3D>();
	}

	void RigidbodyNode3D::begin_play()
	{
		TransformNode3D::begin_play();
	}

	void RigidbodyNode3D::update(const double delta_time)
	{
		TransformNode3D::update(delta_time);
	}

	void RigidbodyNode3D::physics_update(const double delta_time)
	{
		TransformNode3D::physics_update(delta_time);
	}

	void RigidbodyNode3D::end_play()
	{
		TransformNode3D::end_play();
	}
}
