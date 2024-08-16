#include "puffin/nodes/rendering/cameranode3d.h"

#include "puffin/components/rendering/3d/cameracomponent3d.h"

namespace puffin::rendering
{
	CameraNode3D::CameraNode3D(const std::shared_ptr<puffin::core::Engine>& engine, const puffin::UUID& id)
		: TransformNode3D(engine, id)
	{
		m_name = "Camera";

		add_component<CameraComponent3D>();
	}

	bool CameraNode3D::is_active() const
	{
		return get_component<CameraComponent3D>().active;
	}

	void CameraNode3D::set_active(bool active)
	{
		m_registry->patch<CameraComponent3D>(m_entity, [&active](auto& camera) { camera.active = active; });
	}
}
