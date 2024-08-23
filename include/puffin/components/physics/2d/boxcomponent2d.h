#pragma once

#include "nlohmann/json.hpp"

#include "puffin/components/physics/2d/shapecomponent2d.h"
#include "puffin/types/vector2.h"
#include "puffin/utility/reflection.h"
#include "puffin/utility/serialization.h"

namespace puffin
{
	namespace physics
	{
		struct BoxComponent2D : ShapeComponent2D
		{
			BoxComponent2D() = default;

			explicit BoxComponent2D(const Vector2f& halfExtent) : halfExtent(halfExtent) {}

			Vector2f halfExtent = { 0.5f };

			NLOHMANN_DEFINE_TYPE_INTRUSIVE(BoxComponent2D, centreOfMass, halfExtent)
		};
	}

	template<>
	inline void reflection::RegisterType<physics::BoxComponent2D>()
	{
		using namespace physics;

		entt::meta<BoxComponent2D>()
			.type(entt::hs("BoxComponent2D"))
			.data<&BoxComponent2D::centreOfMass>(entt::hs("centreOfMass"))
			.data<&BoxComponent2D::halfExtent>(entt::hs("halfExtent"));
	}

	namespace serialization
	{
		template<>
		inline void Serialize<physics::BoxComponent2D>(const physics::BoxComponent2D& data, Archive& archive)
		{
			archive.Set("centreOfMass", data.centreOfMass);
			archive.Set("halfExtent", data.halfExtent);
		}

		template<>
		inline void Deserialize<physics::BoxComponent2D>(const Archive& archive, physics::BoxComponent2D& data)
		{
			archive.Get("centreOfMass", data.centreOfMass);
			archive.Get("halfExtent", data.halfExtent);
		}
	}
}
