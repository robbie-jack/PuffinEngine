#include "scene/scene_graph.h"

#include "scene/transform_node_2d.h"
#include "scene/transform_node_3d.h"
#include "Core/Engine.h"

namespace puffin::scene
{
	SceneGraph::SceneGraph(const std::shared_ptr<core::Engine>& engine) : System(engine)
	{
		mEngine->registerCallback(core::ExecutionStage::BeginPlay, [&] { begin_play(); }, "SceneGraph: begin_play");
		mEngine->registerCallback(core::ExecutionStage::Update, [&] { update(); }, "SceneGraph: update");
		mEngine->registerCallback(core::ExecutionStage::FixedUpdate, [&] { physics_update(); }, "SceneGraph: physics_update");
		mEngine->registerCallback(core::ExecutionStage::EndPlay, [&] { end_play(); }, "SceneGraph: end_play");

		m_scene_graph_updated = true;
	}

	void SceneGraph::register_default_nodes()
	{
		register_node_type<Node>();
		register_node_type<TransformNode2D>();
		register_node_type<TransformNode3D>();
	}

	void SceneGraph::begin_play()
	{
		for (auto& node : m_nodes_sorted)
		{
			node->update(mEngine->deltaTime());
		}
	}

	void SceneGraph::update()
	{
		if (m_scene_graph_updated)
		{
			m_nodes_sorted.clear();
			m_nodes_sorted.reserve(m_nodes_unsorted.size());

			for (auto& node : m_nodes_unsorted)
			{
				
			}

			m_scene_graph_updated = false;
		}

		for (auto& node : m_nodes_sorted)
		{
			if (node->should_update())
				node->update(mEngine->deltaTime());
		}
	}

	void SceneGraph::physics_update()
	{
		for (auto& node : m_nodes_sorted)
		{
			if (node->should_update())
				node->physics_update(mEngine->timeStepFixed());
		}
	}

	void SceneGraph::end_play()
	{
		for (auto& node : m_nodes_sorted)
		{
			node->end_play();
		}
	}
}
