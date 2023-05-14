#pragma once

#include "Broadphase2D.h"

#include <unordered_map>
#include <unordered_set>

namespace puffin::physics
{
	using SpatialKey = int64_t;

	class SpatialHashBroadphase2D : public Broadphase
	{
	public:

		SpatialHashBroadphase2D() = default;
		~SpatialHashBroadphase2D() override = default;

		void generateCollisionPairs(PackedVector<std::shared_ptr<collision2D::Collider2D>>& inColliders,
			std::vector<CollisionPair>& outCollisionPairs, bool collidersUpdated) override;

	private:

		std::unordered_map<SpatialKey, std::unordered_set<PuffinID>> mColliderSpatialMap;

		const double mCellSize = 4.0;
		const double mCellOffsetSize = 2.0;

		[[nodiscard]] SpatialKey hash(double x, double y) const;

		void getHashIDsForCollider(const std::shared_ptr<collision2D::Collider2D>& collider, std::unordered_set<SpatialKey>& hashIDs) const;

		void updateSpatialMap(PackedVector<std::shared_ptr<collision2D::Collider2D>>& colliders);

	};
}