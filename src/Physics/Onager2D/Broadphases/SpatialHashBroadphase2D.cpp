#include "Physics/Onager2D/Broadphases/SpatialHashBroadphase2D.hpp"

#include "Physics/Onager2D/PhysicsHelpers2D.h"

constexpr int P1 = 73856093;
constexpr int P2 = 83492791;

void Puffin::Physics::SpatialHashBroadphase2D::GenerateCollisionPairs(
	PackedVector<std::shared_ptr<Collision2D::Collider2D>>& inColliders, std::vector<CollisionPair>& outCollisionPairs,
	bool collidersUpdated)
{
	outCollisionPairs.clear();
	outCollisionPairs.reserve(inColliders.Size() * inColliders.Size());

	if (collidersUpdated)
	{
		m_colliderSpatialMap.clear();
	}

	UpdateSpatialMap(inColliders);

	// Generate Collision Pairs
	outCollisionPairs.clear();
	outCollisionPairs.reserve(inColliders.Size() * inColliders.Size());

	for (const auto& [fst, snd] : m_colliderSpatialMap)
	{
		for (const auto& colliderA : snd)
		{
			for (const auto& colliderB : snd)
			{
				auto collisionPair = std::make_pair(colliderA, colliderB);

				if (FilterCollisionPair(collisionPair, outCollisionPairs) == true)
				{
					if (Collision2D::TestAABBVsAABB(colliderA->GetAABB(), colliderB->GetAABB()))
					{
						outCollisionPairs.emplace_back(collisionPair);
					}
				}
			}
		}
	}
}

int Puffin::Physics::SpatialHashBroadphase2D::GenerateHash(double x, double y) const
{
	int ix = static_cast<int>(std::floor(x / m_gridSize));
	int iy = static_cast<int>(std::floor(y / m_gridSize));

	return (ix * P1 ^ iy * P2) % m_hashMapSize;
}

void Puffin::Physics::SpatialHashBroadphase2D::UpdateSpatialMap(PackedVector<std::shared_ptr<Collision2D::Collider2D>>& colliders)
{
	for (const auto& collider : colliders)
	{
		int hashID = GenerateHash(collider->position.x, collider->position.y);

		m_colliderSpatialMap[hashID].push_back(collider);
	}
}
