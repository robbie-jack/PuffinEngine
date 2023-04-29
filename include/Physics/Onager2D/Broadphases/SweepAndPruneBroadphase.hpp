#pragma once

#include "Broadphase2D.hpp"

namespace Puffin::Physics
{
	class SweepAndPruneBroadphase : public Broadphase
	{
	public:

		SweepAndPruneBroadphase() = default;
		~SweepAndPruneBroadphase() override = default;

		void GenerateCollisionPairs(PackedVector<std::shared_ptr<Collision2D::Collider2D>>& inColliders,
			std::vector<CollisionPair>& outCollisionPairs, bool collidersUpdated) override;

	private:

		std::vector<std::shared_ptr<Collision2D::Collider2D>> m_sortedColliders;

		void SortCollidersByX(std::vector<std::shared_ptr<Collision2D::Collider2D>>& colliders);

		bool IsAABBOverlapping(std::shared_ptr<Collision2D::Collider2D> a, std::shared_ptr<Collision2D::Collider2D> b);

	};
}