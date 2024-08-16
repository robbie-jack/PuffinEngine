#pragma once

#include "puffin/nodes/transformnode3d.h"

namespace puffin::rendering
{
	class CameraNode3D : public TransformNode3D
	{
	public:

		explicit CameraNode3D(const std::shared_ptr<puffin::core::Engine>& engine, const puffin::UUID& id = puffin::gInvalidId);
		~CameraNode3D() override = default;

		[[nodiscard]] bool is_active() const;
		void set_active(bool active);

	private:



	};
}
