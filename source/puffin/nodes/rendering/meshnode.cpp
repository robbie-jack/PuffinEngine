#include "puffin/nodes/rendering/meshnode.h"

#include "puffin/components/rendering/3d/staticmeshcomponent3d.h"

namespace puffin::rendering
{
	MeshNode::MeshNode(const std::shared_ptr<core::Engine>& engine, const UUID& id) : TransformNode3D(engine, id)
	{
		m_name = "Mesh";

		add_component<StaticMeshComponent3D>();
	}

	void MeshNode::begin_play()
	{
		TransformNode3D::begin_play();
	}

	void MeshNode::update(const double delta_time)
	{
		TransformNode3D::update(delta_time);
	}

	void MeshNode::update_fixed(const double delta_time)
	{
		TransformNode3D::update_fixed(delta_time);
	}

	void MeshNode::end_play()
	{
		TransformNode3D::end_play();
	}

	UUID MeshNode::mesh_asset_id()
	{
		return get_component<StaticMeshComponent3D>().meshID;
	}

	void MeshNode::set_mesh_asset_id(UUID meshID) const
	{
		m_registry->patch<StaticMeshComponent3D>(m_entity, [&meshID](auto& mesh) { mesh.meshID = meshID; });
	}

	UUID MeshNode::mat_asset_id()
	{
		return get_component<StaticMeshComponent3D>().materialID;
	}

	void MeshNode::set_mat_asset_id(UUID materialID) const
	{
		m_registry->patch<StaticMeshComponent3D>(m_entity, [&materialID](auto& mesh) { mesh.materialID = materialID; });
	}

	uint8_t MeshNode::sub_mesh_idx()
	{
		return get_component<StaticMeshComponent3D>().subMeshIdx;
	}

	void MeshNode::set_sub_mesh_idx(uint8_t subMeshIdx) const
	{
		m_registry->patch<StaticMeshComponent3D>(m_entity, [&subMeshIdx](auto& mesh) { mesh.subMeshIdx = subMeshIdx; });
	}
}
