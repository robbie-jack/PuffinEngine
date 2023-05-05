#pragma once

#include "Broadphase2D.h"

#include <unordered_map>

namespace puffin::physics
{
	class SpatialHashBroadphase2D : public Broadphase
	{
	public:

		SpatialHashBroadphase2D() = default;
		~SpatialHashBroadphase2D() override = default;

		void generateCollisionPairs(PackedVector<std::shared_ptr<collision2D::Collider2D>>& inColliders,
			std::vector<CollisionPair>& outCollisionPairs, bool collidersUpdated) override;

	private:

		typedef std::vector<std::shared_ptr<collision2D::Collider2D>> ColliderVector;

		std::unordered_map<int, ColliderVector> mColliderSpatialMap;

		const int mDefaultGridSize = 1;
		int mGridSize = mDefaultGridSize;

		const int mDefaultHashMapSize = 19;
		int mHashMapSize = mDefaultHashMapSize;

		[[nodiscard]] int generateHash(double x, double y) const;

		void updateSpatialMap(PackedVector<std::shared_ptr<collision2D::Collider2D>>& colliders);

	};
}