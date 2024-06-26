#include "puffin/nodes/rendering/mesh_node.h"

#include "puffin/components/rendering/mesh_component.h"

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

	void MeshNode::update_fixed(const double delta_time)
	{
		TransformNode3D::update_fixed(delta_time);
	}

	void MeshNode::end_play()
	{
		TransformNode3D::end_play();
	}

	PuffinID MeshNode::mesh_asset_id()
	{
		return get_component<MeshComponent>().mesh_asset_id;
	}

	void MeshNode::set_mesh_asset_id(PuffinID mesh_asset_id) const
	{
		m_registry->patch<MeshComponent>(m_entity, [&mesh_asset_id](auto& mesh) { mesh.mesh_asset_id = mesh_asset_id; });
	}

	PuffinID MeshNode::mat_asset_id()
	{
		return get_component<MeshComponent>().mat_asset_id;
	}

	void MeshNode::set_mat_asset_id(PuffinID mat_asset_id) const
	{
		m_registry->patch<MeshComponent>(m_entity, [&mat_asset_id](auto& mesh) { mesh.mat_asset_id = mat_asset_id; });
	}

	uint8_t MeshNode::sub_mesh_idx()
	{
		return get_component<MeshComponent>().sub_mesh_idx;
	}

	void MeshNode::set_sub_mesh_idx(uint8_t sub_mesh_idx) const
	{
		m_registry->patch<MeshComponent>(m_entity, [&sub_mesh_idx](auto& mesh) { mesh.sub_mesh_idx = sub_mesh_idx; });
	}
}
