#pragma once

#include "box2d/b2_circle_shape.h"
#include "box2d/b2_polygon_shape.h"

#include "Types/Vector.h"

#include "nlohmann/json.hpp"

namespace Puffin::Physics
{
	// Serialized Shape Data Structs

	struct SerializedBoxData
	{
		Vector2f halfExtent = Vector2f(1.0f);

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(SerializedBoxData, halfExtent)
	};

	struct SerializedCircleData
	{
		float radius = 1.0f;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(struct SerializedCircleData, radius)
	};

	// Shape Components

	struct Box2DCircleComponent
	{
		b2CircleShape* shape = nullptr;
		SerializedCircleData data;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(Box2DCircleComponent, data)
	};

	struct Box2DBoxComponent
	{
		b2PolygonShape* shape = nullptr;
		SerializedBoxData data;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(Box2DBoxComponent, data)
	};
}