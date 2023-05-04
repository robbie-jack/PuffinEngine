
#include "Physics/Onager2D/Broadphases/SweepAndPruneBroadphase.hpp"

#include "Physics/Onager2D/PhysicsHelpers2D.h"

namespace puffin::physics
{
	void SweepAndPruneBroadphase::generateCollisionPairs(
		PackedVector<std::shared_ptr<collision2D::Collider2D>>& inColliders,
		std::vector<CollisionPair>& outCollisionPairs, bool collidersUpdated)
	{
		if (collidersUpdated)
		{
			sortedColliders_.clear();
			sortedColliders_.reserve(inColliders.Size());

			for (const auto& collider : inColliders)
			{
				sortedColliders_.push_back(collider);
			}
		}

		sortCollidersByX(sortedColliders_);

		for (const auto& colliderA : sortedColliders_)
		{
			for (const auto& colliderB : sortedColliders_)
			{
				AABB a = colliderA->getAABB();
				AABB b = colliderB->getAABB();

				if (a.min.x < b.max.x)
				{
					auto collisionPair = std::make_pair(colliderA, colliderB);

					if (filterCollisionPair(collisionPair, outCollisionPairs) == true)
					{
						if (collision2D::testAabbVsAabb(a, b))
						{
							outCollisionPairs.emplace_back(collisionPair);
						}
					}
				}
			}
		}
	}

	void SweepAndPruneBroadphase::sortCollidersByX(std::vector<std::shared_ptr<collision2D::Collider2D>>& colliders)
	{
		std::sort(colliders.begin(), colliders.end(),
			[=](std::shared_ptr<collision2D::Collider2D> a, std::shared_ptr<collision2D::Collider2D> b) -> bool
			{ return a->getAABB().min.x < b->getAABB().min.x; });
	}
}
