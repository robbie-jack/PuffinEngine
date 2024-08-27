#include "puffin/rendering/vulkan/rendermodule/forward3drendermodulevk.h"

#include "puffin/rendering/rendersubsystem.h"
#include "puffin/rendering/vulkan/rendergraph/rendergraphvk.h"
#include "puffin/rendering/vulkan/rendergraph/renderpassvk.h"

namespace puffin::rendering
{
	Forward3DRenderModuleVK::Forward3DRenderModuleVK(RenderSubsystemVK* renderSubsystem)
		: RenderModuleVK(renderSubsystem)
	{

	}

	void Forward3DRenderModuleVK::Initialize()
	{
		
	}

	void Forward3DRenderModuleVK::Deinitialize()
	{

	}

	void Forward3DRenderModuleVK::Update(double deltaTime)
	{
		
	}

	void Forward3DRenderModuleVK::PrepareGraph(RenderGraphVK& renderGraph)
	{
		auto& forwardPass = renderGraph.AddRenderPass("forward3d", RenderPassType::Graphics);


	}
}
