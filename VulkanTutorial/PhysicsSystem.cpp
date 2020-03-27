#include "PhysicsSystem.h"

void PhysicsSystem::Init()
{
	running = true;
}

bool PhysicsSystem::Update(float dt)
{
	return running;
}

void PhysicsSystem::SendMessage()
{

}

PhysicsSystem::~PhysicsSystem()
{

}