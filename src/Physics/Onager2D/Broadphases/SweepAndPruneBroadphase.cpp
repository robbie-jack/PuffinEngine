
#include "Physics/Onager2D/Broadphases/SweepAndPruneBroadphase.hpp"

#include "Physics/Onager2D/PhysicsHelpers2D.h"

namespace Puffin::Physics
{
	void SweepAndPruneBroadphase::GenerateCollisionPairs(
		PackedVector<std::shared_ptr<Collision2D::Collider2D>>& inColliders,
		std::vector<CollisionPair>& outCollisionPairs, bool collidersUpdated)
	{
		outCollisionPairs.clear();
		outCollisionPairs.reserve(inColliders.Size() * inColliders.Size());

		if (collidersUpdated)
		{
			m_sortedColliders.clear();
			m_sortedColliders.reserve(inColliders.Size());

			for (const auto& collider : inColliders)
			{
				m_sortedColliders.push_back(collider);
			}
		}

		SortCollidersByX(m_sortedColliders);

		for (const auto& colliderA : m_sortedColliders)
		{
			for (const auto& colliderB : m_sortedColliders)
			{
				AABB a = colliderA->GetAABB();
				AABB b = colliderB->GetAABB();

				if (a.min.x < b.max.x)
				{
					auto collisionPair = std::make_pair(colliderA, colliderB);

					if (FilterCollisionPair(collisionPair, outCollisionPairs) == true)
					{
						if (Collision2D::TestAABBVsAABB(a, b))
						{
							outCollisionPairs.emplace_back(collisionPair);
						}
					}
				}
			}
		}
	}

	void SweepAndPruneBroadphase::SortCollidersByX(std::vector<std::shared_ptr<Collision2D::Collider2D>>& colliders)
	{
		std::sort(colliders.begin(), colliders.end(),
			[=](std::shared_ptr<Collision2D::Collider2D> a, std::shared_ptr<Collision2D::Collider2D> b) -> bool
			{ return a->GetAABB().min.x < b->GetAABB().min.x; });
	}
}
