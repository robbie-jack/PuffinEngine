#include "puffin/rendering/vulkan/rendermodule/forward3drendermodulevk.h"

#include "puffin/rendering/rendersubsystem.h"
#include "puffin/rendering/vulkan/rendersubsystemvk.h"
#include "puffin/rendering/vulkan/resourcemanagervk.h"
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
		auto resourceManager = mRenderSubsystem->GetResourceManager();

		AttachmentDescVK color;
		color.format = vk::Format::eR8G8B8A8Unorm;
		color.type = AttachmentTypeVK::Color;

		mColorResourceID = resourceManager->CreateOrUpdateAttachment(color, "forward3d-color");

		AttachmentDescVK depth;
		depth.format = vk::Format::eD32Sfloat;
		color.type = AttachmentTypeVK::Depth;

		mDepthResourceID = resourceManager->CreateOrUpdateAttachment(depth, "forward3d-depth");
	}

	void Forward3DRenderModuleVK::Deinitialize()
	{
		auto resourceManager = mRenderSubsystem->GetResourceManager();

		resourceManager->DestroyResource(mColorResourceID);
		resourceManager->DestroyResource(mDepthResourceID);

		mColorResourceID = gInvalidID;
		mDepthResourceID = gInvalidID;
	}

	void Forward3DRenderModuleVK::UpdateResources(ResourceManagerVK* resourceManager)
	{

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
