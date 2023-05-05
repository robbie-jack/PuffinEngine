#pragma once

#include "box2d/b2_world_callbacks.h"
#include "Physics/CollisionEvent.h"
#include "Types/RingBuffer.h"

namespace puffin::physics
{
	class Box2DContactListener : public b2ContactListener
	{
	public:

		Box2DContactListener() = default;
		~Box2DContactListener() override
		{
			mCollisionBeginEvents.Flush();
			mCollisionEndEvents.Flush();
		}

		void BeginContact(b2Contact* contact) override;
		void EndContact(b2Contact* contact) override;
		void PreSolve(b2Contact* contact, const b2Manifold* oldManifold) override;
		void PostSolve(b2Contact* contact, const b2ContactImpulse* impulse) override;

		bool getNextCollisionBeginEvent(CollisionBeginEvent& collisionBeginEvent);
		bool getNextCollisionEndEvent(CollisionEndEvent& collisionEndEvent);

		void clearEvents();

	private:

		RingBuffer<CollisionBeginEvent> mCollisionBeginEvents;
		RingBuffer<CollisionEndEvent> mCollisionEndEvents;

	};
}