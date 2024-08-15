#pragma once

#include <unordered_map>
#include <unordered_set>

#include "puffin/physics/onager2d/broadphases/broadphase2d.h"
#include "puffin/physics/onager2d/colliders/collider2d.h"
#include "puffin/types/packedvector.h"

namespace puffin::physics
{
	using SpatialKey = int32_t;

	class SpatialHashBroadphase2D : public Broadphase
	{
	public:

		SpatialHashBroadphase2D() = default;
		~SpatialHashBroadphase2D() override = default;

		void generateCollisionPairs(PackedVector<PuffinID, std::shared_ptr<collision2D::Collider2D>>& inColliders,
			std::vector<CollisionPair>& outCollisionPairs, bool collidersUpdated) override;

	private:

		std::unordered_map<SpatialKey, std::unordered_set<PuffinID>> mColliderSpatialMap;

		const double mCellSize = 10.0;
		const double mCellOffsetSize = 1.0;

		[[nodiscard]] SpatialKey hash(const double& x, const double& y) const;

		void getHashIDsForCollider(const std::shared_ptr<collision2D::Collider2D>& collider, std::unordered_set<SpatialKey>& hashIDs) const;

		void updateSpatialMap(PackedVector<PuffinID, std::shared_ptr<collision2D::Collider2D>>& colliders);

	};
}
