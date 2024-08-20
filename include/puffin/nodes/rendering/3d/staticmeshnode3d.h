#pragma once

#include "puffin/nodes/transformnode3d.h"

namespace puffin::rendering
{
	class StaticMeshNode3D : public TransformNode3D
	{
	public:

		explicit StaticMeshNode3D(const std::shared_ptr<core::Engine>& engine, const UUID& id = gInvalidID);
		~StaticMeshNode3D() override = default;

		void Initialize() override;
		void Deinitialize() override;

		UUID GetMeshID();
		void SetMeshID(UUID meshID) const;

		UUID GetMaterialID();
		void SetMaterialID(UUID materialID) const;

		uint8_t GetSubMeshIdx();
		void SetSubMeshIdx(uint8_t subMeshIdx) const;

	private:



	};
}