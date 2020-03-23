#include "RenderSystem.h"

void RenderSystem::Init()
{
	renderer.Init();
}

bool RenderSystem::Update(float dt)
{
	return renderer.Update(dt);
}

void RenderSystem::SendMessage()
{

}

RenderSystem::~RenderSystem()
{
	renderer.Cleanup();
}