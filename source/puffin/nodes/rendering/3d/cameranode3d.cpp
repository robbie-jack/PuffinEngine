#include "puffin/nodes/rendering/3d/cameranode3d.h"

#include "puffin/components/rendering/3d/cameracomponent3d.h"

namespace puffin::rendering
{
	CameraNode3D::CameraNode3D(const std::shared_ptr<puffin::core::Engine>& engine, const puffin::UUID& id)
		: TransformNode3D(engine, id)
	{
		mName = "Camera";

		AddComponent<CameraComponent3D>();
	}

	bool CameraNode3D::GetIsActive() const
	{
		return GetComponent<CameraComponent3D>().active;
	}

	void CameraNode3D::SetIsActive(bool active)
	{
		mRegistry->patch<CameraComponent3D>(mEntity, [&active](auto& camera) { camera.active = active; });
	}
}
