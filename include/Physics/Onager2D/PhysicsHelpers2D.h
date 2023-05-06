#pragma once

#ifndef PHYSICS_HELPERS_2D_H
#define PHYSICS_HELPERS_2D_H

#include "Physics/Onager2D/Colliders/BoxCollider2D.h"
#include "Physics/Onager2D/Colliders/CircleCollider2D.h"

#include "Components/Physics/RigidbodyComponent2D.h"
#include "Components/Physics/ShapeComponents2D.h"

#include "MathHelpers.h"

#include <cmath>
#include <limits>

namespace puffin::physics
{
	namespace collision2D
	{	
		////////////////////////////////
		// GJK Support Functions
		////////////////////////////////

		// Support function to Generate Minkowski Difference/Sum for GJK Algorithm
		static inline Vector2f findSupport(const Collider2D* colliderA, const Collider2D* colliderB, const Vector2f direction)
		{
			return colliderA->findFurthestPoint(direction) - colliderB->findFurthestPoint(-direction);
		}

		static inline bool sameDirection(const Vector2f& direction1, const Vector2f& direction2)
		{
			return direction1.dot(direction2) >= 0.0f;
		}

		static inline Vector2f getPerpendicularDirection(const Vector2f& ab, const Vector2f& direction)
		{
			const Vector2f abClockwise = ab.perpendicularClockwise();
			const Vector2f abCounterClockwise = ab.perpendicularCounterClockwise();

			if (sameDirection(abClockwise, direction))
				return abClockwise;
			else
				return abCounterClockwise;
		}

		static inline bool addSupport(Simplex2D& points, const Collider2D* colliderA, const Collider2D* colliderB, Vector2f direction)
		{
			if (points.size() == 3)
				return true;

			const Vector2f supportPoint = findSupport(colliderA, colliderB, direction);

			points.pushFront(supportPoint);

			const bool isSameDirection = sameDirection(supportPoint, direction);
			return isSameDirection;
		}

		// Find Vector Perpendicular to ab in direction of origin
		static inline bool findLine(const Simplex2D& points, Vector2f& direction)
		{
			const Vector2f a = points[0];
			const Vector2f b = points[1];

			const Vector2f ab = b - a;
			const Vector2f ao = -a;

			// Get perpendicular vector facing towards origin
			direction = getPerpendicularDirection(ab, ao);

			return false;
		}

		// Check if origin is within Triangle
		static inline bool findTriangle(Simplex2D& points, Vector2f& direction)
		{
			Vector2f a = points[0];
			Vector2f b = points[1];
			Vector2f c = points[2];

			const Vector2f ab = b - a;
			const Vector2f ac = c - a;
			const Vector2f ao = -a;

			const Vector2f abp = getPerpendicularDirection(ab, -ac);
			const Vector2f acp = getPerpendicularDirection(ac, -ab);

			// Is origin beyond ab in direction of ab perpendicular
			if (sameDirection(abp, ao))
			{
				points = { a, b };
				direction = abp;
			}
			// Is origin beyond ac in direction of ac perpendicular
			else if (sameDirection(acp, ao))
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

		static inline bool nextSimplex(Simplex2D& points, Vector2f& direction)
		{
			switch (points.size())
			{
				case 2: return findLine(points, direction);
				case 3: return findTriangle(points, direction);
			default: ;
			}

			return false;
		}

		// GJK Algorithm to see if any two shapes with minkowski support function collide
		static inline bool gjk(const Collider2D* colliderA, const Collider2D* colliderB, Simplex2D& points)
		{
			Vector2f direction = (colliderB->position - colliderA->position).normalized();
			const Vector2f supportPoint = findSupport(colliderA, colliderB, direction);

			points.pushFront(supportPoint);

			direction *= -1;

			for (int i = 0; i < 32; i++)
			{
				if (!addSupport(points, colliderA, colliderB, direction))
				{
					return false;
				}

				if (nextSimplex(points, direction))
				{
					return true;
				}
			}

			return false;
		}

		////////////////////////////////
		// EPA Support Functions
		////////////////////////////////

		static inline Edge findClosestEdge(const Polygon2D& polygon)
		{
			Edge edge;

			// Get maximum value of distance
			edge.distance = std::numeric_limits<float>::max();

			for (int i = 0; i < polygon.size(); i++)
			{
				// Compute next points index;
				const int j = (i + 1) % polygon.size();

				// Get Current point and next one
				Vector2f a = polygon[i];
				Vector2f b = polygon[j];

				// Create edge vector
				Vector2f ab = b - a;

				// Get Vector from origin to a
				Vector2f ao = -a;

				// Get Vector from edge towards origin
				Vector2f normal = ab.perpendicularClockwise().normalized();

				// Calculate distance from origin to egde
				float distance = normal.dot(a);

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

		static inline void epa(const Collider2D* colliderA, const Collider2D* colliderB, Contact& contact, const Simplex2D& simplex)
		{
			auto polygon = Polygon2D(simplex);

			// Loop to find collision information
			for (int i = 0; i < 32; i++)
			{
				// Obtain feature (edge for 2D) closest to
				// origin on the minkowski difference
				Edge edge = findClosestEdge(polygon);

				// Obtain a new support point in the direction of the edge normal
				Vector2f point = findSupport(colliderA, colliderB, edge.normal);

				// Check distance from origin to edge against
				// distance point is along edge normal
				const float distance = edge.normal.dot(point);
				const float tolerance = 0.001f;

				const float diff = std::abs(distance - edge.distance);
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
					contact.normal = edge.normal;
					contact.seperation = distance;

					return;
				}
			}
		}

		////////////////////////////////
		// Collision Test Functions
		////////////////////////////////

		static inline bool testBoxVsBox(const BoxCollider2D* boxA, const BoxCollider2D* boxB, Contact& outContact)
		{
			Simplex2D points;

			if (!gjk(boxA, boxB, points))
				return false;

			outContact.a = boxA->uuid;
			outContact.b = boxB->uuid;

			epa(boxA, boxB, outContact, points);

			outContact.pointOnA = boxA->position + outContact.normal * dynamic_cast<BoxShape2D*>(boxA->shape)->halfExtent;
			outContact.pointOnB = boxB->position - outContact.normal * dynamic_cast<BoxShape2D*>(boxB->shape)->halfExtent;

			// No separating axis found, shapes must be colliding
			return true;
		}

		static inline bool testCircleVsCircle(const CircleCollider2D* circleA, const CircleCollider2D* circleB, Contact& outContact)
		{
			// Non-GJK Circle Collision Test
			// Vector from A to B
			const Vector2 ab = circleB->position - circleA->position;

			// Get Cumulative Radius of Circles
			const float radiusAB = circleA->shape->radius + circleB->shape->radius;
			const float radiusABSq = radiusAB * radiusAB;

			if (ab.lengthSquared() > radiusABSq)
				return false;

			outContact.a = circleA->uuid;
			outContact.b = circleB->uuid;

			outContact.normal = ab.normalized();

			// GJK Circle Collision Test
			/*Simplex2D points;

			if (!GJK(circleA, circleB, points))
				return false;

			outContact.a = circleA->entity;
			outContact.b = circleB->entity;

			EPA(circleA, circleB, outContact, points);*/

			outContact.pointOnA = circleA->position + outContact.normal * circleA->shape->radius;
			outContact.pointOnB = circleB->position - outContact.normal * circleB->shape->radius;

			return true;
		}

		static inline bool testCircleVsBox(const CircleCollider2D* circleA, const BoxCollider2D* boxB, Contact& outContact)
		{
			Simplex2D points;

			if (!gjk(circleA, boxB, points))
				return false;

			outContact.a = circleA->uuid;
			outContact.b = boxB->uuid;

			epa(circleA, boxB, outContact, points);

			outContact.pointOnA = circleA->position + outContact.normal * circleA->shape->radius;
			outContact.pointOnB = boxB->position - outContact.normal * dynamic_cast<BoxShape2D*>(boxB->shape)->halfExtent;

			return true;
		}

		static inline bool testAabbVsAabb(const AABB& a, const AABB& b)
		{
			return a.min.x < b.max.x&& a.max.x > b.min.x && a.min.y < b.max.y&& a.max.y > b.min.y;
		}
	}

	////////////////////////////////
	// Impulse Helper Functions
	////////////////////////////////

	static void applyLinearImpulse(RigidbodyComponent2D& body, const Vector2f& impulse)
	{
		if (body.mass == 0.0f)
			return;

		// Apply Accumulated Impulse
		body.linearVelocity += impulse * body.mass;
	}

	static void applyAngularImpulse(RigidbodyComponent2D& body, const float& impulse)
	{
		if (body.mass == 0.0f)
			return;

		const float impulseMultMass = impulse * body.mass;
		const float as = std::asin(impulseMultMass);

		body.angularVelocity += as * (180 / 3.14);

		constexpr float maxAngularSpeed = 30.0f;

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

	static void applyImpulse(RigidbodyComponent2D& body, const Vector2f& impulsePoint, const Vector2f& impulse)
	{
		if (body.mass == 0.0f)
			return;

		// impulsePoint is the world space location of the application of the impulse
		// impulse is the world space direction and magnitude of the impulse
		applyLinearImpulse(body, impulse);

		// Get the 2D cross of impulsePoint against impulse
		// this is the sin of the angle between the vectors in radians
		//ApplyAngularImpulse(body, impulsePoint.Normalised().Cross(impulse.Normalised()));
	}
}

#endif //PHYSICS_HELPERS_2D_H
