#pragma once

#ifndef PHYSICS_HELPERS_2D_H
#define PHYSICS_HELPERS_2D_H

#include "Physics/Colliders/BoxCollider2D.h"
#include "Physics/Colliders/CircleCollider2D.h"

#include <Components/Physics/RigidbodyComponent2D.h>
#include <Components/Physics/ShapeComponents2D.h>

#include "MathHelpers.h"

#include <cmath>
#include <memory>

namespace Puffin::Physics
{
	namespace Collision2D
	{		
		// Support function to Generate Minkowski Difference/Sum for GJK Algorithm
		static inline Vector2f Support(const Collider2D* colliderA, const Collider2D* colliderB, Vector2f direction)
		{
			return colliderA->FindFurthestPoint(direction) - colliderB->FindFurthestPoint(-direction);
		}

		static inline bool SameDirection(const Vector2f& direction1, const Vector2f& direction2)
		{
			return direction1.Dot(direction2) >= 0.0f;
		}

		static inline Vector2f GetPerpendicularDirection(const Vector2f& ab, const Vector2f& direction)
		{
			Vector2f abcw = ab.PerpendicularClockwise();
			Vector2f abccw = ab.PerpendicularCounterClockwise();

			if (SameDirection(abcw, direction))
				return abcw;
			else
				return abccw;
		}

		static inline bool AddSupport(Simplex2D& points, const Collider2D* colliderA, const Collider2D* colliderB, Vector2f direction)
		{
			if (points.size() == 3)
				return true;

			Vector2f support = Support(colliderA, colliderB, direction);

			points.push_front(support);

			bool isSameDirection = SameDirection(support, direction);
			return isSameDirection;
		}

		static inline bool Line(Simplex2D& points, Vector2f& direction)
		{
			Vector2f a = points[0];
			Vector2f b = points[1];

			Vector2f ab = b - a;
			Vector2f ao = -a;

			// If origin is not in same direction as ab, discard b
			direction = GetPerpendicularDirection(ab, ao);

			return false;
		}

		static inline bool Triangle(Simplex2D& points, Vector2f& direction)
		{
			Vector2f a = points[0];
			Vector2f b = points[1];
			Vector2f c = points[2];

			Vector2f ab = b - a;
			Vector2f ac = c - a;
			Vector2f ao = -a;

			Vector2f abp = GetPerpendicularDirection(ab, -ac);
			Vector2f acp = GetPerpendicularDirection(ac, -ab);

			// If Vector Perpendicular to AC in direction of Origin?
			if (SameDirection(abp, ao))
			{
				points = { a, b };
				direction = abp;
			}
			else if (SameDirection(acp, ao))
			{
				// Is Vector Perpendicular to AB in dirtection of Origin
				points = { a, c };
				direction = acp;
			}
			else
			{
				return true;
			}

			return false;
		}

		static inline bool NextSimplex(Simplex2D& points, Vector2f& direction)
		{
			switch (points.size())
			{
				case 2: return Line(points, direction);
				case 3: return Triangle(points, direction);
			}

			return false;
		}

		// GJK Algorithm to see if any two shapes with minkoski support funciton collide
		static inline bool GJK(const Collider2D* colliderA, const Collider2D* colliderB)
		{
			Vector2f direction = (colliderB->transform.position.GetXY() - colliderA->transform.position.GetXY()).Normalised();
			Vector2f support = Support(colliderA, colliderB, direction);

			Simplex2D points;
			points.push_front(support);

			direction *= -1;

			int maxIterations = 10;
			int currentIterations = 1;
			while (currentIterations <= maxIterations)
			{
				if (!AddSupport(points, colliderA, colliderB, direction))
				{
					return false;
				}

				if (NextSimplex(points, direction))
				{
					return true;
				}

				currentIterations++;
			}
		}

		// Collection of Functions to test for collisions between 2D Shapes

		static inline bool TestBoxVsBox(const BoxCollider2D* boxA, const BoxCollider2D* boxB, Contact& outContact)
		{
			if (!GJK(boxA, boxB))
				return false;

			outContact.a = boxA->entity;
			outContact.b = boxB->entity;

			// No separating axis found, shapes must be colliding
			return true;
		}

		static inline bool TestCircleVsCircle(const CircleCollider2D* circleA, const CircleCollider2D* circleB, Contact& outContact)
		{
			// Vector from A to B
			const Vector2 ab = circleB->transform.position.GetXY() - circleA->transform.position.GetXY();

			// Get Cumulative Radius of Circles
			const float radiusAB = circleA->shape->radius + circleB->shape->radius;
			const float radiusABSq = radiusAB * radiusAB;

			if (ab.LengthSquared() > radiusABSq)
				return false;

			outContact.a = circleA->entity;
			outContact.b = circleB->entity;

			outContact.normal = ab.Normalised();

			outContact.pointOnA = circleA->transform.position.GetXY() + outContact.normal * circleA->shape->radius;
			outContact.pointOnB = circleB->transform.position.GetXY() - outContact.normal * circleB->shape->radius;

			return true;
		}

		static inline bool TestCircleVsBox(const CircleCollider2D* circleA, const BoxCollider2D* boxB, Contact& outContact)
		{
			// Get Circle/Box Centres
			

			return false;
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
