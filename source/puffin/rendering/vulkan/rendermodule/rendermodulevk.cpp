#include "puffin/rendering/vulkan/rendermodule/rendermodulevk.h"

namespace puffin::rendering
{
	RenderModuleVK::RenderModuleVK(std::shared_ptr<core::Engine> engine, RenderSubsystemVK* renderSubsystem)
		: mEngine(std::move(engine)), mRenderSubsystem(renderSubsystem)
	{
	}

	RenderModuleVK::~RenderModuleVK()
	{
		mRenderSubsystem = nullptr;
		mEngine = nullptr;
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

	void RenderModuleVK::PostInitialize()
	{
	}

	void RenderModuleVK::UpdateResources(ResourceManagerVK* resourceManager)
	{
	}

	void RenderModuleVK::UpdateGraph(RenderGraphVK& renderGraph)
	{
	}

	void RenderModuleVK::PreRender(double deltaTime)
	{
	}
}
