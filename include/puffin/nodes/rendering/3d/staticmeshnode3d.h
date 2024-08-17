#pragma once

#include "puffin/nodes/transformnode3d.h"

namespace puffin::rendering
{
	class MeshNode : public TransformNode3D
	{
	public:

		explicit MeshNode(const std::shared_ptr<core::Engine>& engine, const UUID& id = gInvalidID);

		UUID GetMeshID();
		void SetMeshID(UUID meshID) const;

		UUID GetMaterialID();
		void SetMaterialID(UUID materialID) const;

		uint8_t GetSubMeshIdx();
		void SetSubMeshIdx(uint8_t subMeshIdx) const;

	private:



	};
}