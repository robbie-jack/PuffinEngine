#pragma once

#include "box2d/b2_body.h"

#include "nlohmann/json.hpp"

	NLOHMANN_JSON_SERIALIZE_ENUM(b2BodyType,
	{
		{b2_staticBody, "static"},
		{b2_kinematicBody, "kinematic"},
		{b2_dynamicBody, "dynamic"}
	})

	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(b2BodyDef, type, allowSleep, bullet, awake, enabled, fixedRotation,
		angularDamping, linearDamping, gravityScale)

	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(b2FixtureDef, density, friction, restitution, restitutionThreshold)

namespace Puffin::Physics
{
	struct Box2DRigidbodyComponent
	{
		Box2DRigidbodyComponent()
		{
			fixtureDef.density = 1.0f;
			fixtureDef.restitution = 0.5f;
			fixtureDef.friction = 0.5f;
		}

		~Box2DRigidbodyComponent()
		{
			body = nullptr;
		}

		b2Body* body = nullptr;
		b2BodyDef bodyDef;
		b2Fixture* fixture = nullptr;
		b2FixtureDef fixtureDef;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(Box2DRigidbodyComponent, bodyDef, fixtureDef)
	};
}