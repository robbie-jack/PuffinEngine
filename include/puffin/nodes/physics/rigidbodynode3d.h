#pragma once

#include "puffin/nodes/transformnode3d.h"
#include "puffin/utility/reflection.h"

namespace puffin
{
	namespace physics
	{
		class RigidbodyNode3D : public TransformNode3D
		{
		public:

			explicit RigidbodyNode3D(const std::shared_ptr<core::Engine>& engine, const UUID& id = gInvalidID);
			~RigidbodyNode3D() override = default;

		};
	}

	template<>
	inline void reflection::RegisterType<physics::RigidbodyNode3D>()
	{
		using namespace physics;

		entt::meta<RigidbodyNode3D>()
			.type(entt::hs("RigidbodyNode3D"))
			.base<TransformNode3D>();
	}
}