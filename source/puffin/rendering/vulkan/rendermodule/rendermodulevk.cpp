#include "puffin/rendering/vulkan/rendermodule/rendermodulevk.h"

namespace puffin::rendering
{
	RenderModuleVK::RenderModuleVK(RenderSubsystemVK* renderSubsystem)
		: mRenderSubsystem(renderSubsystem)
	{
	}

	RenderModuleVK::~RenderModuleVK()
	{
		mRenderSubsystem = nullptr;
	}

	void RenderModuleVK::Initialize()
	{
	}

	void RenderModuleVK::Deinitialize()
	{
	}

	void RenderModuleVK::Update(double deltaTime)
	{

	}

	void RenderModuleVK::PrepareGraph(RenderGraphVK& renderGraph)
	{

	}
}
