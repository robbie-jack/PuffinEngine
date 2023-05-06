
#include "Physics\Onager2D\Broadphases\SweepAndPruneBroadphase.h"

#include "Physics/Onager2D/PhysicsHelpers2D.h"

namespace puffin::physics
{
	void SweepAndPruneBroadphase::generateCollisionPairs(
		PackedVector<std::shared_ptr<collision2D::Collider2D>>& inColliders,
		std::vector<CollisionPair>& outCollisionPairs, bool collidersUpdated)
	{
		if (collidersUpdated)
		{
			mSortedColliders.clear();
			mSortedColliders.reserve(inColliders.size());

			for (const auto& collider : inColliders)
			{
				mSortedColliders.push_back(collider);
			}
		}

		sortCollidersByX(mSortedColliders);

		for (const auto& colliderA : mSortedColliders)
		{
			for (const auto& colliderB : mSortedColliders)
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
