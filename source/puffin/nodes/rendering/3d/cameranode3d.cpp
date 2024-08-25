#include "puffin/nodes/rendering/3d/cameranode3d.h"

#include "puffin/components/rendering/3d/cameracomponent3d.h"

namespace puffin::rendering
{
	CameraNode3D::CameraNode3D(const std::shared_ptr<puffin::core::Engine>& engine, const puffin::UUID& id)
		: TransformNode3D(engine, id)
	{
		mName = "Camera";
	}

	void CameraNode3D::Initialize()
	{
		TransformNode3D::Initialize();

		AddComponent<CameraComponent3D>();
	}

	void CameraNode3D::Deinitialize()
	{
		TransformNode3D::Deinitialize();

		RemoveComponent<CameraComponent3D>();
	}

	const std::string& CameraNode3D::GetTypeString() const
	{
		return gCameraNode3DTypeString;
	}

	entt::id_type CameraNode3D::GetTypeID() const
	{
		return gCameraNode3DTypeID;
	}

	bool CameraNode3D::GetActive() const
	{
		return GetComponent<CameraComponent3D>().active;
	}

	bool& CameraNode3D::Active()
	{
		mRegistry->patch<CameraComponent3D>(mEntity);

		return GetComponent<CameraComponent3D>().active;
	}

	void CameraNode3D::SetActive(bool active)
	{
		mRegistry->patch<CameraComponent3D>(mEntity, [&active](auto& camera) { camera.active = active; });
	}
}
