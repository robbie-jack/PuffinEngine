#pragma once

#include "System.h"

class PhysicsSystem : public System
{
public:

	void Init();
	bool Update(float dt);
	void SendMessage();

	~PhysicsSystem();

private:


};