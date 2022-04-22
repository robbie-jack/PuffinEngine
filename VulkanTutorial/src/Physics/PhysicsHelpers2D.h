#pragma once

#ifndef PHYSICS_HELPERS_2D_H
#define PHYSICS_HELPERS_2D_H

#include "Physics/Colliders/BoxCollider2D.h"
#include "Physics/Colliders/CircleCollider2D.h"

#include <Components/Physics/RigidbodyComponent2D.h>
#include <Components/Physics/ShapeComponents2D.h>

#include "MathHelpers.h"

#include <cmath>

namespace Puffin::Physics
{
	namespace Collision2D
	{
		// Collection of Functions to test for collisions between 2D Shapes

		inline bool TestBoxVsBox(const BoxCollider2D* boxA, const BoxCollider2D* boxB, Contact& outContact)
		{
			// Get Min/Max bounds of each box
			const AABB a = boxA->GetAABB();
			const AABB b = boxB->GetAABB();

			// Exit with no intersection if found with separating axis
			if (a.max.x < b.min.x || a.min.x > b.max.x) return false;
			if (a.max.y < b.min.y || a.min.y > b.max.y) return false;

			outContact.a = boxA->entity_;
			outContact.b = boxB->entity_;

			// No separating axis found, shapes must be colliding
			return true;
		}

		inline bool TestCircleVsCircle(const CircleCollider2D* circleA, const CircleCollider2D* circleB, Contact& outContact)
		{
			// Vector from A to B
			const Vector2 ab = circleB->transform_.position.GetXY() - circleA->transform_.position.GetXY();

			// Get Cumulative Radius of Circles
			const float radiusAB = circleA->shape_->radius_ + circleB->shape_->radius_;
			const float radiusABSq = radiusAB * radiusAB;

			if (ab.LengthSquared() > radiusABSq)
				return false;

			outContact.a = circleA->entity_;
			outContact.b = circleB->entity_;

			outContact.normal = ab.Normalised();

			outContact.pointOnA = circleA->transform_.position.GetXY() + outContact.normal * circleA->shape_->radius_;
			outContact.pointOnB = circleB->transform_.position.GetXY() - outContact.normal * circleB->shape_->radius_;

			return true;
		}

		inline bool TestCircleVsBox(const CircleCollider2D* circleA, const BoxCollider2D* boxB, Contact& outContact)
		{
			// Get Circle/Box Centres
			Vector2f centreCircle = circleA->transform_.position.GetXY() + circleA->shape_->radius_;
			Vector2f centreBox = boxB->transform_.position.GetXY() + boxB->shape_->halfExtent_;

			// Get Clamped Diff between Centres
			Vector2f diff = centreCircle - centreBox;
			Vector2f diffClamped = Maths::Clamp(diff, Vector2f(-boxB->shape_->halfExtent_.x, -boxB->shape_->halfExtent_.y), boxB->shape_->halfExtent_);

			// Add clamped value to Box centre to get point of box closest to circle
			const Vector2f closest = centreBox + diffClamped;

			// Get diff between centreCircle and closest point
			diff = closest - centreCircle;

			outContact.a = circleA->entity_;
			outContact.b = boxB->entity_;

			// If length of diff is less than Circle radius, then Circle and Box are colliding
			return diff.LengthSquared() < circleA->shape_->radius_ * circleA->shape_->radius_;
		}
	}

	// Apply Impulse

	static inline void ApplyLinearImpulse(RigidbodyComponent2D& body, const Vector2f& impulse)
	{
		if (body.invMass == 0.0f)
			return;

		// Apply Accumulated Impulse
		body.linearVelocity += impulse * body.invMass;
	}

	static inline void ApplyAngularImpulse(RigidbodyComponent2D& body, const float& impulse)
	{
		if (body.invMass == 0.0f)
			return;

		body.angularVelocity += std::asin(impulse * body.invMass) * (180 / 3.14);
	}

	static inline void ApplyImpulse(RigidbodyComponent2D& body, const Vector2f& impulsePoint, const Vector2f& impulse)
	{
		if (body.invMass == 0.0f)
			return;

		// impulsePoint is the world space location of the application of the impulse
		// impulse is the world space direction and magnitude of the impulse
		ApplyLinearImpulse(body, impulse);

		// Get the 2D cross of impulsePoint against impulse
		// this is the sin of the angle between the vectors in radians
		ApplyAngularImpulse(body, impulsePoint.Cross(impulse));
	}
}

#endif //PHYSICS_HELPERS_2D_H
