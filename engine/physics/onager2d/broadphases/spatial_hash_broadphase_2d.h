#pragma once

#include <unordered_map>
#include <unordered_set>

#include "physics/onager2d/broadphases/broadphase_2d.h"
#include "physics/onager2d/colliders/collider_2d.h"
#include "types/storage/mapped_vector.h"

namespace puffin::physics
{
	using SpatialKey = int32_t;

	class SpatialHashBroadphase2D : public Broadphase
	{
	public:

		SpatialHashBroadphase2D() = default;
		~SpatialHashBroadphase2D() override = default;

		void generateCollisionPairs(MappedVector<UUID, std::shared_ptr<collision2D::Collider2D>>& inColliders,
			std::vector<CollisionPair>& outCollisionPairs, bool collidersUpdated) override;

	private:

		std::unordered_map<SpatialKey, std::unordered_set<UUID>> mColliderSpatialMap;

		const double mCellSize = 10.0;
		const double mCellOffsetSize = 1.0;

		[[nodiscard]] SpatialKey hash(const double& x, const double& y) const;

		void getHashIDsForCollider(const std::shared_ptr<collision2D::Collider2D>& collider, std::unordered_set<SpatialKey>& hashIDs) const;

		void updateSpatialMap(MappedVector<UUID, std::shared_ptr<collision2D::Collider2D>>& colliders);

	};
}
