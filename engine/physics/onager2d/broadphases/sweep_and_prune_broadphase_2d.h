#pragma once

#include "physics/onager2d/broadphases/broadphase_2d.h"
#include "physics/onager2d/colliders/collider_2d.h"
#include "types/storage/mapped_vector.h"

namespace puffin::physics
{
	class SweepAndPruneBroadphase : public Broadphase
	{
	public:

		SweepAndPruneBroadphase() = default;
		~SweepAndPruneBroadphase() override = default;

		void generateCollisionPairs(MappedVector<UUID, std::shared_ptr<collision2D::Collider2D>>& inColliders,
			std::vector<CollisionPair>& outCollisionPairs, bool collidersUpdated) override;

	private:

		std::vector<std::shared_ptr<collision2D::Collider2D>> mSortedColliders;

		void sortCollidersByX(std::vector<std::shared_ptr<collision2D::Collider2D>>& colliders);

	};
}
