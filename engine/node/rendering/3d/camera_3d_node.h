#pragma once

#include "node/transform_3d_node.h"
#include "utility/reflection.h"

namespace puffin
{
	namespace rendering
	{
		const std::string gCameraNode3DTypeString = "CameraNode3D";
		const entt::id_type gCameraNode3DTypeID = entt::hs(gCameraNode3DTypeString.c_str());

		class CameraNode3D : public TransformNode3D
		{
		public:

			void Initialize() override;
			void Deinitialize() override;

			[[nodiscard]] std::string_view GetTypeString() const override;
			[[nodiscard]] entt::id_type GetTypeID() const override;

			[[nodiscard]] bool GetActive() const;
			[[nodiscard]] bool& Active();
			void SetActive(bool active);

		private:



		};
	}

	template<>
	inline void reflection::RegisterType<rendering::CameraNode3D>()
	{
		using namespace rendering;

		entt::meta<CameraNode3D>()
			.type(gCameraNode3DTypeID)
			.base<TransformNode3D>()
			.custom<NodeCustomData>(gCameraNode3DTypeString);
	}
}
