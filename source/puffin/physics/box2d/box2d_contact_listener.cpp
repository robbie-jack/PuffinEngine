#include "puffin/physics/box2d/box2d_contact_listener.h"

#if PFN_BOX2D_PHYSICS

//#include "box2d/b2_contact.h"
//
//#include "puffin/physics/collision_event.h"
//
//namespace puffin::physics
//{
//	void Box2DContactListener::BeginContact(b2Contact* contact)
//	{
//		CollisionBeginEvent collisionBeginEvent;
//		collisionBeginEvent.entityA = contact->GetFixtureA()->GetBody()->GetUserData().pointer;
//		collisionBeginEvent.entityB = contact->GetFixtureB()->GetBody()->GetUserData().pointer;
//
//		mCollisionBeginEvents.push(collisionBeginEvent);
//	}
//
//	void Box2DContactListener::EndContact(b2Contact* contact)
//	{
//		CollisionEndEvent collisionEndEvent;
//		collisionEndEvent.entityA = contact->GetFixtureA()->GetBody()->GetUserData().pointer;
//		collisionEndEvent.entityB = contact->GetFixtureB()->GetBody()->GetUserData().pointer;
//
//		mCollisionEndEvents.push(collisionEndEvent);
//	}
//
//	void Box2DContactListener::PreSolve(b2Contact* contact, const b2Manifold* oldManifold)
//	{
//		
//	}
//
//	void Box2DContactListener::PostSolve(b2Contact* contact, const b2ContactImpulse* impulse)
//	{
//		
//	}
//
//	bool Box2DContactListener::getNextCollisionBeginEvent(CollisionBeginEvent& collisionBeginEvent)
//	{
//		return mCollisionBeginEvents.pop(collisionBeginEvent);
//	}
//
//	bool Box2DContactListener::getNextCollisionEndEvent(CollisionEndEvent& collisionEndEvent)
//	{
//		return mCollisionEndEvents.pop(collisionEndEvent);
//	}
//
//	void Box2DContactListener::clearEvents()
//	{
//		mCollisionBeginEvents.flush();
//		mCollisionEndEvents.flush();
//	}
//}

#endif