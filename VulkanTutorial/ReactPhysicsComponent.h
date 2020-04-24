#pragma once

#include "BaseComponent.h"

#include "reactphysics3d.h"

using namespace reactphysics3d;

class ReactPhysicsComponent : public BaseComponent
{
public:

	ReactPhysicsComponent();
	~ReactPhysicsComponent();

	inline rp3d::RigidBody* GetBody() { return body; };
	inline void SetBody(rp3d::RigidBody* body_) { body = body_; };

	inline rp3d::Transform GetTransform() { return body->getTransform(); };

	inline rp3d::Transform GetPrevTransform() { return prevTransform; };
	inline void SetPrevTransform(rp3d::Transform transform) { prevTransform = transform; };

	inline rp3d::Transform GetLerpTransform() { return lerpTransform; };
	inline void SetLerpTransform(rp3d::Transform transform) { lerpTransform = transform; };

private:

	rp3d::RigidBody* body;
	rp3d::Transform lerpTransform;
	rp3d::Transform prevTransform;
};