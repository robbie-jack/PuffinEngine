#pragma once

#include "Broadphase2D.h"

namespace puffin::physics
{
	class SweepAndPruneBroadphase : public Broadphase
	{
	public:

		SweepAndPruneBroadphase() = default;
		~SweepAndPruneBroadphase() override = default;

		void generateCollisionPairs(PackedVector<std::shared_ptr<collision2D::Collider2D>>& inColliders,
			std::vector<CollisionPair>& outCollisionPairs, bool collidersUpdated) override;

	private:

		std::vector<std::shared_ptr<collision2D::Collider2D>> mSortedColliders;

		void sortCollidersByX(std::vector<std::shared_ptr<collision2D::Collider2D>>& colliders);

	};
}