#include "puffin/physics/onager2d/broadphases/spatial_hash_broadphase_2d.h"

#include <memory>

#include "puffin/physics/onager2d/physics_helpers_2d.h"
#include "puffin/physics/onager2d/broadphases/broadphase_2d.h"
#include "puffin/physics/onager2d/colliders/collider_2d.h"
#include "puffin/types/packed_array.h"

void puffin::physics::SpatialHashBroadphase2D::generateCollisionPairs(
	PackedVector<std::shared_ptr<collision2D::Collider2D>>& inColliders, std::vector<CollisionPair>& outCollisionPairs,
	bool collidersUpdated)
{
	outCollisionPairs.clear();
	outCollisionPairs.reserve(inColliders.size() * inColliders.size());

	updateSpatialMap(inColliders);

	// Generate Collision Pairs
	outCollisionPairs.clear();
	outCollisionPairs.reserve(inColliders.size() * inColliders.size());

	for (const auto& colliderA : inColliders)
	{
		std::unordered_set<SpatialKey> hashIDs;

		getHashIDsForCollider(colliderA, hashIDs);

		for (const auto& hashID : hashIDs)
		{
			for (const auto& colliderID : mColliderSpatialMap[hashID])
			{
				const auto& colliderB = inColliders[colliderID];

				if (auto collisionPair = std::make_pair(colliderA, colliderB); filterCollisionPair(collisionPair, outCollisionPairs) == true)
				{
					if (collision2D::testAabbVsAabb(colliderA->getAABB(), colliderB->getAABB()))
					{
						outCollisionPairs.emplace_back(collisionPair);
					}
				}
			}
		}
	}
}

puffin::physics::SpatialKey puffin::physics::SpatialHashBroadphase2D::hash(const double& x, const double& y) const
{
	const auto ix = static_cast<SpatialKey>(std::floor(x / mCellSize));
	const auto iy = static_cast<SpatialKey>(std::floor(y / mCellSize));

	return ix ^ iy;
}

void puffin::physics::SpatialHashBroadphase2D::getHashIDsForCollider(
	const std::shared_ptr<collision2D::Collider2D>& collider, std::unordered_set<SpatialKey>& hashIDs) const
{
	hashIDs.clear();

	const auto aabb = collider->getAABB();

	for (double x = aabb.min.x; x <= aabb.max.x;)
	{
		for (double y = aabb.min.y; y <= aabb.max.y;)
		{
			hashIDs.emplace(hash(x, y));

			y += mCellOffsetSize;
		}

		x += mCellOffsetSize;
	}
}

void puffin::physics::SpatialHashBroadphase2D::updateSpatialMap(PackedVector<std::shared_ptr<collision2D::Collider2D>>& colliders)
{
	mColliderSpatialMap.clear();

	for (const auto& collider : colliders)
	{
		std::unordered_set<SpatialKey> hashIDs;

		getHashIDsForCollider(collider, hashIDs);

		for (const auto& id : hashIDs)
		{
			mColliderSpatialMap[id].emplace(collider->uuid);
		}
	}
}
