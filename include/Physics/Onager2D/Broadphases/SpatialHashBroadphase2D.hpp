#pragma once

#include "Broadphase2D.hpp"

#include <unordered_map>

namespace Puffin::Physics
{
	class SpatialHashBroadphase2D : public Broadphase
	{
	public:

		SpatialHashBroadphase2D() = default;
		~SpatialHashBroadphase2D() override = default;

		void GenerateCollisionPairs(PackedVector<std::shared_ptr<Collision2D::Collider2D>>& inColliders,
			std::vector<CollisionPair>& outCollisionPairs, bool collidersUpdated) override;

	private:

		typedef std::vector<std::shared_ptr<Collision2D::Collider2D>> ColliderVector;

		std::unordered_map<int, ColliderVector> m_colliderSpatialMap;

		const int m_defaultGridSize = 1;
		int m_gridSize = m_defaultGridSize;

		const int m_defaultHashMapSize = 19;
		int m_hashMapSize = m_defaultHashMapSize;

		int GenerateHash(double x, double y) const;

		void UpdateSpatialMap(PackedVector<std::shared_ptr<Collision2D::Collider2D>>& colliders);

	};
}