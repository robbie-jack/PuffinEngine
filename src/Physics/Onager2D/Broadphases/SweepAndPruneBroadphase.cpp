
#include "Physics/Onager2D/Broadphases/SweepAndPruneBroadphase.hpp"

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

		for (int i = 0; i < m_sortedColliders.size(); i++)
		{
			for (int j = i + 1; j < m_sortedColliders.size(); i++)
			{

			}
		}
	}

	void SweepAndPruneBroadphase::SortCollidersByX(std::vector<std::shared_ptr<Collision2D::Collider2D>>& colliders)
	{
		std::sort(colliders.begin(), colliders.end(),
			[=](std::shared_ptr<Collision2D::Collider2D> a, std::shared_ptr<Collision2D::Collider2D> b) -> bool
			{ return a->GetAABB().min.x < b->GetAABB().min.x; });
	}

	bool SweepAndPruneBroadphase::IsAABBOverlapping(std::shared_ptr<Collision2D::Collider2D> a,
		std::shared_ptr<Collision2D::Collider2D> b)
	{
		return false;
	}
}
