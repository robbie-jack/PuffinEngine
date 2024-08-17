#include "puffin/nodes/rendering/3d/staticmeshnode3d.h"

#include "puffin/components/rendering/3d/staticmeshcomponent3d.h"

namespace puffin::rendering
{
	MeshNode::MeshNode(const std::shared_ptr<core::Engine>& engine, const UUID& id) : TransformNode3D(engine, id)
	{
		mName = "Mesh";

		AddComponent<StaticMeshComponent3D>();
	}

	UUID MeshNode::GetMeshID()
	{
		return GetComponent<StaticMeshComponent3D>().meshID;
	}

	void MeshNode::SetMeshID(UUID meshID) const
	{
		mRegistry->patch<StaticMeshComponent3D>(mEntity, [&meshID](auto& mesh) { mesh.meshID = meshID; });
	}

	UUID MeshNode::GetMaterialID()
	{
		return GetComponent<StaticMeshComponent3D>().materialID;
	}

	void MeshNode::SetMaterialID(UUID materialID) const
	{
		mRegistry->patch<StaticMeshComponent3D>(mEntity, [&materialID](auto& mesh) { mesh.materialID = materialID; });
	}

	uint8_t MeshNode::GetSubMeshIdx()
	{
		return GetComponent<StaticMeshComponent3D>().subMeshIdx;
	}

	void MeshNode::SetSubMeshIdx(uint8_t subMeshIdx) const
	{
		mRegistry->patch<StaticMeshComponent3D>(mEntity, [&subMeshIdx](auto& mesh) { mesh.subMeshIdx = subMeshIdx; });
	}
}
