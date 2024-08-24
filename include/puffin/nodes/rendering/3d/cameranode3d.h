#pragma once

#include "puffin/nodes/transformnode3d.h"
#include "puffin/utility/reflection.h"

namespace puffin
{
	namespace rendering
	{
		class CameraNode3D : public TransformNode3D
		{
		public:

			explicit CameraNode3D(const std::shared_ptr<puffin::core::Engine>& engine, const puffin::UUID& id = puffin::gInvalidID);
			~CameraNode3D() override = default;

			void Initialize() override;
			void Deinitialize() override;

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
			.type(entt::hs("CameraNode3D"))
			.base<TransformNode3D>();
	}
}
