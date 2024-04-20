#include "scene/scene_graph.h"

#include "scene/transform_node_2d.h"
#include "scene/transform_node_3d.h"
#include "Core/Engine.h"

namespace puffin::scene
{
	SceneGraph::SceneGraph(const std::shared_ptr<core::Engine>& engine) : System(engine)
	{
		mEngine->registerCallback(core::ExecutionStage::SubsystemUpdate, [&] { subsystem_update(); }, "SceneGraph: subsystem_update");
		mEngine->registerCallback(core::ExecutionStage::Update, [&] { update(); }, "SceneGraph: update");
		mEngine->registerCallback(core::ExecutionStage::FixedUpdate, [&] { physics_update(); }, "SceneGraph: physics_update");
		mEngine->registerCallback(core::ExecutionStage::EndPlay, [&] { end_play(); }, "SceneGraph: end_play");

		m_scene_graph_updated = true;
	}

	void SceneGraph::subsystem_update()
	{
		update_scene_graph();

		update_global_transforms();
	}

	void SceneGraph::update()
	{
		for (auto& id : m_node_ids)
		{
			if (const auto node = get_node_ptr(id); node &&  node->should_update())
				node->update(mEngine->deltaTime());
		}
	}

	void SceneGraph::physics_update()
	{
		for (auto& id : m_node_ids)
		{
			if (const auto node = get_node_ptr(id); node && node->should_update())
				node->physics_update(mEngine->timeStepFixed());
		}
	}

	void SceneGraph::end_play()
	{
		for (auto& id : m_node_ids)
		{
			if (const auto node = get_node_ptr(id); node)
				node->end_play();
		}
	}

	void SceneGraph::update_scene_graph()
	{
		if (m_scene_graph_updated)
		{
			m_node_ids.clear();

			for (auto& id : m_root_node_ids)
			{
				m_node_ids.push_back(id);

				const auto node = get_node_ptr(id);

				std::vector<PuffinID> child_ids;
				node->get_child_ids(child_ids);

				m_node_ids.insert(m_node_ids.end(), child_ids.begin(), child_ids.end());
			}

			m_scene_graph_updated = false;
		}
	}

	void SceneGraph::register_default_nodes()
	{
		register_node_type<Node>();
		register_node_type<TransformNode2D>();
		register_node_type<TransformNode3D>();
	}

	void SceneGraph::update_global_transforms()
	{
		// PUFFIN_TODO - Implement global transform updates
	}
}
