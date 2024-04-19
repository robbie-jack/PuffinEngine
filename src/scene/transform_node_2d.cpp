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

	void TransformNode2D::update(double delta_time)
	{

	}

	void TransformNode2D::physics_update(double delta_time)
	{

	}

	void TransformNode2D::end_play()
	{

	}

	TransformComponent2D& TransformNode2D::get_transform()
	{
		return get_component<TransformComponent2D>();
	}
}
