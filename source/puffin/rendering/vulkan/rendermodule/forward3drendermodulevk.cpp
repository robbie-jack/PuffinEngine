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

	void Forward3DRenderModuleVK::RegisterModules()
	{
		
	}

	void Forward3DRenderModuleVK::Initialize()
	{
		
	}

	void Forward3DRenderModuleVK::Deinitialize()
	{

	}

	void Forward3DRenderModuleVK::DefineResources(ResourceManagerVK* resourceManager)
	{
		ImageDescVK color;
		color.format = vk::Format::eR8G8B8A8Unorm;
		color.usageFlags = { vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled |
				vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst };


	}

	void Forward3DRenderModuleVK::BuildGraph(RenderGraphVK& renderGraph)
	{
		

		auto& forwardPass = renderGraph.AddRenderPass("forward3d", RenderPassType::Graphics);
		forwardPass.AddOutputColorAttachment("forward3d-color");
		forwardPass.SetOutputDepthStencilAttachment("forward3d-depth");

		forwardPass.SetRecordCommandsCallback([this](vk::CommandBuffer& cmd)
		{
			RecordForward3DCommands(cmd);
		});

	}

	void Forward3DRenderModuleVK::PreRender(double deltaTime)
	{

	}

	void Forward3DRenderModuleVK::RecordForward3DCommands(vk::CommandBuffer& cmd)
	{
		//	1. Begin Rendering



		//	2. Set Draw Parameters (Viewport, Scissor, etc...)



		//	3. Bind Buffers & Descriptors



		//	4. Render Mesh Batches

		//		4a. Bind Pipeline or Batch



		//		4b. Draw Batch



		//	5. End Rendering


	}
}
