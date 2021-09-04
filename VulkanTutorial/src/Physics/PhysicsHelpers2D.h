#pragma once

#ifndef PHYSICS_HELPERS_2D_H
#define PHYSICS_HELPERS_2D_H

#include <Components/TransformComponent.h>
#include <Components/Physics/RigidbodyComponent2D.h>
#include <Components/Physics/ShapeComponent2D.h>

#include <MathHelpers.h>
#include <ECS/ECS.h>

#include <Types/Matrix.h>
#include <Types/PhysicsTypes2D.h>

#include <algorithm>
#include <cmath>

namespace Puffin
{
	namespace Physics
	{
		namespace Collision2D
		{
			struct AABB
			{
				Vector2 min;
				Vector2 max;
			};

			// Collection of Functions to test for collisions between 2D Shapes

			// Get AABB from Entity Transform and Box Shape
			AABB& GetAABB(const TransformComponent& Transform, const ShapeBox& Box)
			{
				AABB aabb;
				aabb.min = Transform.position.GetXY() - Box.halfExtent;
				aabb.max = Transform.position.GetXY() + Box.halfExtent;
				return aabb;
			}

			bool TestBoxVsBox(const TransformComponent& TransformA, const ShapeBox& BoxA, const TransformComponent& TransformB, const ShapeBox& BoxB, Contact& OutContact)
			{
				// Get Min/Max bounds of each box
				AABB a = GetAABB(TransformA, BoxA);
				AABB b = GetAABB(TransformB, BoxB);

				// Exit with no intersection if found with separating axis
				if (a.max.x < b.min.x || a.min.x > b.max.x) return false;
				if (a.max.y < b.min.y || a.min.y > b.max.y) return false;

				// No separating axis found, shapes must be colliding
				return true;
			}

			bool TestCircleVsCircle(const TransformComponent& TransformA, const ShapeCircle& CirlceA, const TransformComponent& TransformB, const ShapeCircle& CirlceB, Contact& OutContact)
			{
				// Vector from A to B
				Vector2 ab = TransformB.position.GetXY() - TransformA.position.GetXY();

				// Get Cumulative Radius of Circles
				float radiusAB = CirlceA.radius + CirlceB.radius;
				float radiusABSq = radiusAB * radiusAB;

				if (ab.LengthSquared() > radiusABSq)
					return false;

				OutContact.normal = ab.Normalised();

				OutContact.pointOnA = TransformA.position.GetXY() + OutContact.normal * CirlceA.radius;
				OutContact.pointOnB = TransformB.position.GetXY() - OutContact.normal * CirlceB.radius;

				return true;
			}

			bool TestCircleVsBox(const TransformComponent& TransformA, const ShapeCircle& CircleA, const TransformComponent& TransformB, const ShapeBox& BoxB, Contact& OutContact)
			{
				// Get Circle/Box Centres
				Vector2 centreCircle = TransformA.position.GetXY() + CircleA.radius;
				Vector2 centreBox = TransformB.position.GetXY() + BoxB.halfExtent;

				// Get Clamped Diff between Centres
				Vector2 diff = centreCircle - centreBox;
				Vector2 diffClamped = Maths::Clamp(diff, Vector2(-BoxB.halfExtent.x, -BoxB.halfExtent.y), BoxB.halfExtent);

				// Add clamped value to Box centre to get point of box closest to circle
				Vector2 closest = centreBox + diffClamped;

				// Get diff between centreCircle and closts point
				diff = closest - centreCircle;

				// If length of diff is less than Circle radius, then Circle and Box are colliding
				return diff.LengthSquared() < CircleA.radius * CircleA.radius;
			}
		}

		// Apply Impulse

		static inline void ApplyLinearImpulse(RigidbodyComponent2D& body, const Vector2& impulse)
		{
			if (body.invMass == 0.0f)
				return;

			// Apply Accumulated Impulse
			body.linearVelocity += impulse * body.invMass;
		}

		static inline void ApplyAngularImpulse(RigidbodyComponent2D& body, const Collision2D::ShapeCircle& circle, const Float& impulse)
		{
			if (body.invMass == 0.0f)
				return;

			body.angularVelocity += std::asin(impulse * body.invMass) * (180 / 3.14);
		}

		static inline void ApplyImpulse(RigidbodyComponent2D& body, const Collision2D::ShapeCircle& circle, const Vector2& impulsePoint, const Vector2& impulse)
		{
			if (body.invMass == 0.0f)
				return;

			// impulsePoint is the world space location of the application of the impulse
			// impulse is the world space direction and magnitude of the impulse
			ApplyLinearImpulse(body, impulse);

			// Get the 2D cross of impulsePoint against impulse
			// this is the sin of the angle between the vectors in radians
			ApplyAngularImpulse(body, circle, impulsePoint.Cross(impulse));
		}
	}
}

#endif //PHYSICS_HELPERS_2D_H