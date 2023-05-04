#include "Physics/Onager2D/Broadphases/SpatialHashBroadphase2D.hpp"

#include "Physics/Onager2D/PhysicsHelpers2D.h"

constexpr int P1 = 73856093;
constexpr int P2 = 83492791;

void puffin::physics::SpatialHashBroadphase2D::generateCollisionPairs(
	PackedVector<std::shared_ptr<collision2D::Collider2D>>& inColliders, std::vector<CollisionPair>& outCollisionPairs,
	bool collidersUpdated)
{
	outCollisionPairs.clear();
	outCollisionPairs.reserve(inColliders.Size() * inColliders.Size());

	if (collidersUpdated)
	{
		colliderSpatialMap_.clear();
	}

	updateSpatialMap(inColliders);

	// Generate Collision Pairs
	outCollisionPairs.clear();
	outCollisionPairs.reserve(inColliders.Size() * inColliders.Size());

	for (const auto& [fst, snd] : colliderSpatialMap_)
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
	const int ix = static_cast<int>(std::floor(x / gridSize_));
	const int iy = static_cast<int>(std::floor(y / gridSize_));

	return (ix * P1 ^ iy * P2) % hashMapSize_;
}

void puffin::physics::SpatialHashBroadphase2D::updateSpatialMap(PackedVector<std::shared_ptr<collision2D::Collider2D>>& colliders)
{
	for (const auto& collider : colliders)
	{
		int hashID = generateHash(collider->position.x, collider->position.y);

		colliderSpatialMap_[hashID].push_back(collider);
	}
}
