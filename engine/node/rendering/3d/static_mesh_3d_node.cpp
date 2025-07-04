#include "node/rendering/3d/static_mesh_3d_node.h"

#include "component/rendering/3d/static_mesh_component_3d.h"

namespace puffin::rendering
{
	void StaticMeshNode3D::Initialize()
	{
		TransformNode3D::Initialize();

		AddComponent<StaticMeshComponent3D>();
	}

	void StaticMeshNode3D::Deinitialize()
	{
		TransformNode3D::Deinitialize();

		RemoveComponent<StaticMeshComponent3D>();
	}

	std::string_view StaticMeshNode3D::GetTypeString() const
	{
		return gStaticMeshNode3DTypeString;
	}

	entt::id_type StaticMeshNode3D::GetTypeID() const
	{
		return gStaticMeshNode3DTypeID;
	}

	UUID StaticMeshNode3D::GetMeshID()
	{
		return GetComponent<StaticMeshComponent3D>().meshID;
	}

	void StaticMeshNode3D::SetMeshID(UUID meshID) const
	{
		mRegistry->patch<StaticMeshComponent3D>(mEntity, [&meshID](auto& mesh) { mesh.meshID = meshID; });
	}

	UUID StaticMeshNode3D::GetMaterialID()
	{
		return GetComponent<StaticMeshComponent3D>().materialID;
	}

	void StaticMeshNode3D::SetMaterialID(UUID materialID) const
	{
		mRegistry->patch<StaticMeshComponent3D>(mEntity, [&materialID](auto& mesh) { mesh.materialID = materialID; });
	}

	uint8_t StaticMeshNode3D::GetSubMeshIdx()
	{
		return GetComponent<StaticMeshComponent3D>().subMeshIdx;
	}

	void StaticMeshNode3D::SetSubMeshIdx(uint8_t subMeshIdx) const
	{
		mRegistry->patch<StaticMeshComponent3D>(mEntity, [&subMeshIdx](auto& mesh) { mesh.subMeshIdx = subMeshIdx; });
	}
}
