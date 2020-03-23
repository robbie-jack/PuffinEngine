#pragma once

#include "System.h"
#include "VulkanRenderer.h"

class RenderSystem : public System
{
public:

	void Init();
	bool Update(float dt);
	void SendMessage();
	~RenderSystem();

private:
	VulkanRenderer renderer;
};