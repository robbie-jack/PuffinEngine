#pragma once

#ifndef PHYSICS_HELPERS_2D_H
#define PHYSICS_HELPERS_2D_H

#include <Components/TransformComponent.h>
#include <Components/Physics/RigidbodyComponent2D.h>
#include <Components/Physics/ShapeComponent2D.h>

#include <MathHelpers.h>
#include <ECS/ECS.h>

#include <Types/Matrix.h>

#include <algorithm>

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

		/*Mat3 GetInverseInertiaTensorBodySpace(const RigidbodyComponent2D& body, const ShapeCircle* circle)
		{
			Mat3 inertiaTensor = GetInertiaTensor(circle);
			Mat3 invInertiaTensor = inertiaTensor.Inverse() * body.invMass;
			return invInertiaTensor;
		};

		Mat3 GetInverseInertiaTensorWorldSpace(const RigidbodyComponent2D& body, const ShapeCircle* circle)
		{
			Mat3 inertiaTensor = GetInertiaTensor(circle);
			Mat3 invInertiaTensor = inertiaTensor.Inverse() * body.invMass;
			Mat3 orient = body.orientation.ToMat3();
			Mat3 invInertiaTensor = orient * invInertiaTensor * orient.Transpose();
			return invInertiaTensor;
		}*/
	}
}


#endif //PHYSICS_HELPERS_2D_H