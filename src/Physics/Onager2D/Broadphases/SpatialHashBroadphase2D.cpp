#include "Physics\Onager2D\Broadphases\SpatialHashBroadphase2D.h"

#include "Physics/Onager2D/PhysicsHelpers2D.h"

constexpr int gP1 = 73856093;
constexpr int gP2 = 83492791;

void puffin::physics::SpatialHashBroadphase2D::generateCollisionPairs(
	PackedVector<std::shared_ptr<collision2D::Collider2D>>& inColliders, std::vector<CollisionPair>& outCollisionPairs,
	bool collidersUpdated)
{
	outCollisionPairs.clear();
	outCollisionPairs.reserve(inColliders.size() * inColliders.size());

	if (collidersUpdated)
	{
		mColliderSpatialMap.clear();
	}

	updateSpatialMap(inColliders);

	// Generate Collision Pairs
	outCollisionPairs.clear();
	outCollisionPairs.reserve(inColliders.size() * inColliders.size());

	for (const auto& [fst, snd] : mColliderSpatialMap)
	{
		for (const auto& colliderA : snd)
		{
			for (const auto& colliderB : snd)
			{
				auto collisionPair = std::make_pair(colliderA, colliderB);

				if (filterCollisionPair(collisionPair, outCollisionPairs) == true)
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

int puffin::physics::SpatialHashBroadphase2D::generateHash(double x, double y) const
{
	const int ix = static_cast<int>(std::floor(x / mGridSize));
	const int iy = static_cast<int>(std::floor(y / mGridSize));

	return (ix * gP1 ^ iy * gP2) % mHashMapSize;
}

void puffin::physics::SpatialHashBroadphase2D::updateSpatialMap(PackedVector<std::shared_ptr<collision2D::Collider2D>>& colliders)
{
	for (const auto& collider : colliders)
	{
		int hashID = generateHash(collider->position.x, collider->position.y);

		mColliderSpatialMap[hashID].push_back(collider);
	}
}
