#include "Rendering/mesh_node.h"
#include "Components/Rendering/MeshComponent.h"

namespace puffin::rendering
{
	MeshNode::MeshNode(const std::shared_ptr<core::Engine>& engine, const PuffinID& id) : TransformNode3D(engine, id)
	{
		m_name = "Mesh";

		add_component<MeshComponent>();
	}

	void MeshNode::begin_play()
	{
		TransformNode3D::begin_play();
	}

	void MeshNode::update(const double delta_time)
	{
		TransformNode3D::update(delta_time);
	}

	void MeshNode::physics_update(const double delta_time)
	{
		TransformNode3D::physics_update(delta_time);
	}

	void MeshNode::end_play()
	{
		TransformNode3D::end_play();
	}

	PuffinID& MeshNode::mesh_asset_id()
	{
		return get_component<MeshComponent>().meshAssetID;
	}

	PuffinID& MeshNode::mat_asset_id()
	{
		return get_component<MeshComponent>().matAssetID;
	}

	uint8_t MeshNode::sub_mesh_idx()
	{
		return get_component<MeshComponent>().subMeshIdx;
	}
}
