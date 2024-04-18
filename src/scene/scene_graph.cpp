#include "scene/scene_graph.h"

#include "scene/transform_node_2d.h"
#include "scene/transform_node_3d.h"

namespace puffin::scene
{
	SceneGraph::SceneGraph(const std::shared_ptr<core::Engine>& engine) : System(engine)
	{

	}

	void SceneGraph::register_default_nodes()
	{
		register_node<TransformNode2D>();
		register_node<TransformNode3D>();
	}
}
