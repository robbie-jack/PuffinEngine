#pragma once

#include "node/transform_3d_node.h"

namespace puffin
{
	namespace rendering
	{
		const std::string gStaticMeshNode3DTypeString = "StaticMeshNode3D";
		const entt::id_type gStaticMeshNode3DTypeID = entt::hs(gStaticMeshNode3DTypeString.c_str());

		class StaticMeshNode3D : public TransformNode3D
		{
		public:

			void Initialize() override;
			void Deinitialize() override;

			[[nodiscard]] std::string_view GetTypeString() const override;
			[[nodiscard]] entt::id_type GetTypeID() const override;

			UUID GetMeshID();
			void SetMeshID(UUID meshID) const;

			UUID GetMaterialID();
			void SetMaterialID(UUID materialID) const;

			uint8_t GetSubMeshIdx();
			void SetSubMeshIdx(uint8_t subMeshIdx) const;

		private:



		};
	}

	template<>
	inline void reflection::RegisterType<rendering::StaticMeshNode3D>()
	{
		using namespace rendering;

		entt::meta<StaticMeshNode3D>()
			.type(gStaticMeshNode3DTypeID)
			.base<TransformNode3D>()
			.custom<NodeCustomData>(gStaticMeshNode3DTypeString);
	}
}