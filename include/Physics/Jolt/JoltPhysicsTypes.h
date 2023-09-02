#pragma once

#include "Jolt/Jolt.h"

#include "Jolt/Physics/Collision/ObjectLayer.h"
#include "Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h"

namespace puffin::physics
{
	// Layer that objects can be in, determines which other objects it can collide with
	// Typically you at least want to have 1 layer for moving bodies and 1 layer for static bodies, but you can have more
	// layers if you want. E.g. you could have a layer for high detail collision (which is not used by the physics simulation
	// but only if you do collision testing).
	namespace JoltLayers
	{
		static constexpr JPH::ObjectLayer NON_MOVING = 0;
		static constexpr JPH::ObjectLayer MOVING = 1;
		static constexpr JPH::ObjectLayer NUM_LAYERS = 2;
	}

	// Each broadphase layer results in a separate bounding volume tree in the broad phase. You at least want to have
	// a layer for non-moving and moving objects to avoid having to update a tree full of static objects every frame.
	// You can have a 1-on-1 mapping between object layers and broadphase layers (like in this case) but if you have
	// many object layers you'll be creating many broad phase trees, which is not efficient. If you want to fine tune
	// your broadphase layers define JPH_TRACK_BROADPHASE_STATS and look at the stats reported on the TTY.
	namespace JoltBroadPhaseLayers
	{
		static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
		static constexpr JPH::BroadPhaseLayer MOVING(1);
		static constexpr JPH::uint NUM_LAYERS(2);
	};

	// Class that determines if two object layers can collide
	class JoltObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter
	{
	public:
		bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override
		{
			switch (inObject1)
			{
			case JoltLayers::NON_MOVING:
				return inObject2 == JoltLayers::MOVING; // Non moving only collides with moving
			case JoltLayers::MOVING:
				return true; // Moving collides with everything
			default:
				//JPH_ASSERT(false);
				return false;
			}
		}
	};

	// BroadPhaseLayerInterface implementation
// This defines a mapping between object and broadphase layers.
	class JoltBPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface
	{
	public:
		JoltBPLayerInterfaceImpl()
		{
			// Create a mapping table from object to broad phase layer
			mObjectToBroadPhase[JoltLayers::NON_MOVING] = JoltBroadPhaseLayers::NON_MOVING;
			mObjectToBroadPhase[JoltLayers::MOVING] = JoltBroadPhaseLayers::MOVING;
		}

		JPH::uint					GetNumBroadPhaseLayers() const override
		{
			return JoltBroadPhaseLayers::NUM_LAYERS;
		}

		JPH::BroadPhaseLayer			GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override
		{
			//JPH_ASSERT(inLayer < JoltLayers::NUM_LAYERS);
			return mObjectToBroadPhase[inLayer];
		}

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
		const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override
		{
			switch ((JPH::BroadPhaseLayer::Type)inLayer)
			{
			case (JPH::BroadPhaseLayer::Type)JoltBroadPhaseLayers::NON_MOVING:	return "NON_MOVING";
			case (JPH::BroadPhaseLayer::Type)JoltBroadPhaseLayers::MOVING:		return "MOVING";
			default:	/*JPH_ASSERT(false);*/ return "INVALID";
			}
		}
#endif // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED

	private:
		JPH::BroadPhaseLayer mObjectToBroadPhase[JoltLayers::NUM_LAYERS] {};
	};

	/// Class that determines if an object layer can collide with a broadphase layer
	class JoltObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter
	{
	public:
		virtual bool				ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override
		{
			switch (inLayer1)
			{
			case JoltLayers::NON_MOVING:
				return inLayer2 == JoltBroadPhaseLayers::MOVING;
			case JoltLayers::MOVING:
				return true;
			default:
				//JPH_ASSERT(false);
				return false;
			}
		}
	};
}