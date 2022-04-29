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
#include <limits>

namespace Puffin::Physics
{
	namespace Collision2D
	{	
		////////////////////////////////
		// GJK Support Functions
		////////////////////////////////

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

		// Find Vector Perpendicular to ab in direction of origin
		static inline bool Line(Simplex2D& points, Vector2f& direction)
		{
			Vector2f a = points[0];
			Vector2f b = points[1];

			Vector2f ab = b - a;
			Vector2f ao = -a;

			// Get perpendicular vector facing towards origin
			direction = GetPerpendicularDirection(ab, ao);

			return false;
		}

		// Check if origin is within Triangle
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

			// Is origin beyond ab in direction of ab perpendicular
			if (SameDirection(abp, ao))
			{
				points = { a, b };
				direction = abp;
			}
			// Is origin beyond ac in direction of ac perpendicular
			else if (SameDirection(acp, ao))
			{
				points = { a, c };
				direction = acp;
			}
			else
			{
				// Origin is beyond neither ac or ab, so it must be within triangle
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
		static inline bool GJK(const Collider2D* colliderA, const Collider2D* colliderB, Simplex2D& points)
		{
			Vector2f direction = (colliderB->transform.position.GetXY() - colliderA->transform.position.GetXY()).Normalised();
			Vector2f support = Support(colliderA, colliderB, direction);

			points.push_front(support);

			direction *= -1;

			for (int i = 0; i < 32; i++)
			{
				if (!AddSupport(points, colliderA, colliderB, direction))
				{
					return false;
				}

				if (NextSimplex(points, direction))
				{
					return true;
				}
			}

			return false;
		}

		////////////////////////////////
		// EPA Support Functions
		////////////////////////////////

		static inline Edge FindClosestEdge(const Polygon2D& polygon)
		{
			Edge edge;

			// Get maximum value of distance
			edge.distance = std::numeric_limits<float>::max();

			for (int i = 0; i < polygon.size(); i++)
			{
				// Compute next points index;
				int j = (i + 1) % polygon.size();

				// Get Current point and next one
				Vector2f a = polygon[i];
				Vector2f b = polygon[j];

				// Create edge vector
				Vector2f ab = b - a;

				// Get Vector from origin to a
				Vector2f ao = -a;

				// Get Vector from edge towards origin
				Vector2f normal = ab.PerpendicularClockwise().Normalised();

				// Calculate distance from origin to egde
				float distance = normal.Dot(a);

				// Check distance against other disances
				if (distance < 0)
				{
					distance *= -1;
					normal *= -1;
				}

				if (distance < edge.distance)
				{
					edge.distance = distance;
					edge.normal = normal;
					edge.index = j;
				}
			}

			// Return closest edge we found
			return edge;
		}

		static inline void EPA(const Collider2D* colliderA, const Collider2D* colliderB, Contact& outContact, const Simplex2D& simplex)
		{
			Polygon2D polygon = Polygon2D(simplex);

			// Loop to find collision information
			for (int i = 0; i < 32; i++)
			{
				// Obtain feature (edge for 2D) closest to
				// origin on the minkowski difference
				Edge edge = FindClosestEdge(polygon);

				// Obtain a new support point in the direction of the edge normal
				Vector2f point = Support(colliderA, colliderB, edge.normal);

				// Check distance from origin to edge against
				// distance point is along edge normal
				float distance = edge.normal.Dot(point);
				float tolerance = 0.001f;

				float diff = std::abs(distance - edge.distance);
				if (diff > tolerance)
				{
					// We haven't reached edge of the Minkowski Difference
					// so continue expanding by adding point to the simplex
					// in between points that made the edge
					polygon.insert(edge.index, point);
				}
				else
				{
					// If differance is less than tolerance then
					// we assume, that we cannot expand the simplex any further
					// and we have our solution
					outContact.normal = edge.normal;
					outContact.seperation = distance;

					return;
				}
			}
		}

		////////////////////////////////
		// Collision Test Functions
		////////////////////////////////

		static inline bool TestBoxVsBox(const BoxCollider2D* boxA, const BoxCollider2D* boxB, Contact& outContact)
		{
			Simplex2D points;

			if (!GJK(boxA, boxB, points))
				return false;

			outContact.a = boxA->entity;
			outContact.b = boxB->entity;

			EPA(boxA, boxB, outContact, points);

			outContact.pointOnA = boxA->transform.position.GetXY() + outContact.normal * std::static_pointer_cast<BoxShape2D>(boxA->shape)->halfExtent;
			outContact.pointOnB = boxB->transform.position.GetXY() - outContact.normal * std::static_pointer_cast<BoxShape2D>(boxB->shape)->halfExtent;

			// No separating axis found, shapes must be colliding
			return true;
		}

		static inline bool TestCircleVsCircle(const CircleCollider2D* circleA, const CircleCollider2D* circleB, Contact& outContact)
		{
			// Non-GJK Circle Collision Test
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

			// GJK Circle Collision Test
			/*Simplex2D points;

			if (!GJK(circleA, circleB, points))
				return false;

			outContact.a = circleA->entity;
			outContact.b = circleB->entity;

			EPA(circleA, circleB, outContact, points);*/

			outContact.pointOnA = circleA->transform.position.GetXY() + outContact.normal * circleA->shape->radius;
			outContact.pointOnB = circleB->transform.position.GetXY() - outContact.normal * circleB->shape->radius;

			return true;
		}

		static inline bool TestCircleVsBox(const CircleCollider2D* circleA, const BoxCollider2D* boxB, Contact& outContact)
		{
			Simplex2D points;

			if (!GJK(circleA, boxB, points))
				return false;

			outContact.a = circleA->entity;
			outContact.b = boxB->entity;

			EPA(circleA, boxB, outContact, points);

			outContact.pointOnA = circleA->transform.position.GetXY() + outContact.normal * circleA->shape->radius;
			outContact.pointOnB = boxB->transform.position.GetXY() - outContact.normal * std::static_pointer_cast<BoxShape2D>(boxB->shape)->halfExtent;

			return true;
		}
	}

	////////////////////////////////
	// Impulse Helper Functions
	////////////////////////////////

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

		float impulseMultMass = impulse * body.invMass;
		float as = std::asin(impulseMultMass);

		body.angularVelocity += as * (180 / 3.14);

		const float maxAngularSpeed = 30.0f;

		// Cap angular speed at 30 deg/s
		if (body.angularVelocity > 0.0)
		{
			body.angularVelocity = std::min(body.angularVelocity, maxAngularSpeed);
		}
		else if (body.angularVelocity < 0.0)
		{
			body.angularVelocity = std::max(body.angularVelocity, -maxAngularSpeed);
		}
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
		ApplyAngularImpulse(body, impulsePoint.Normalised().Cross(impulse.Normalised()));
	}
}

#endif //PHYSICS_HELPERS_2D_H
