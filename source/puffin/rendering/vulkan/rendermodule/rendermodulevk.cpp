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

	void RenderModuleVK::RegisterModules()
	{
	}

	void RenderModuleVK::Initialize()
	{
	}

	void RenderModuleVK::Deinitialize()
	{
	}

	void RenderModuleVK::UpdateResources(ResourceManagerVK* resourceManager)
	{
	}

	void RenderModuleVK::BuildGraph(RenderGraphVK& renderGraph)
	{
	}

	void RenderModuleVK::PreRender(double deltaTime)
	{
	}
}
