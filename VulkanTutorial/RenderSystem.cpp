#include "RenderSystem.h"

void RenderSystem::Init()
{
	renderer.Init();
}

void RenderSystem::Update(float dt)
{
	renderer.Update(dt);
}

void RenderSystem::SendMessage()
{

}

RenderSystem::~RenderSystem()
{
	renderer.Cleanup();
}