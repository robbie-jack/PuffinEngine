#pragma once

#include "Broadphase2D.hpp"

namespace Puffin::Physics
{
	class PruneAndSweepBroadphase : public Broadphase
	{
	public:

		PruneAndSweepBroadphase() = default;
		~PruneAndSweepBroadphase() override = default;

		void GenerateCollisionPairs(PackedVector<std::shared_ptr<Collision2D::Collider2D>>& inColliders, std::vector<CollisionPair>& outCollisionPairs) override;

	private:



	};
}