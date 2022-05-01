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

	// Copy current body properties to bodyDef
	static void UpdateBodyDef(Box2DRigidbodyComponent& rb)
	{
		const auto* body = rb.body;

		rb.bodyDef.type = body->GetType();
		rb.bodyDef.allowSleep = body->IsSleepingAllowed();
		rb.bodyDef.bullet = body->IsBullet();
		rb.bodyDef.awake = body->IsAwake();
		rb.bodyDef.enabled = body->IsEnabled();
		rb.bodyDef.fixedRotation = body->IsFixedRotation();
		rb.bodyDef.angularDamping = body->GetAngularDamping();
		rb.bodyDef.linearDamping = body->GetLinearDamping();
		rb.bodyDef.gravityScale = body->GetGravityScale();
	}

	// Copy current fixture properties to bodyDef
	static void UpdateFixtureDef(Box2DRigidbodyComponent& rb)
	{
		const auto* fixture = rb.fixture;

		rb.fixtureDef.density = fixture->GetDensity();
		rb.fixtureDef.friction = fixture->GetFriction();
		rb.fixtureDef.restitution = fixture->GetRestitution();
		rb.fixtureDef.restitutionThreshold = fixture->GetRestitutionThreshold();
	}
}