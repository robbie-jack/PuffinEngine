#include "puffin/rendering/vulkan/rendermodule/forward3drendermodulevk.h"

#include "puffin/assets/assetregistry.h"
#include "puffin/components/rendering/3d/cameracomponent3d.h"
#include "puffin/rendering/camerasubsystem.h"
#include "puffin/rendering/rendersubsystem.h"
#include "puffin/rendering/vulkan/helpersvk.h"
#include "puffin/rendering/vulkan/materialregistryvk.h"
#include "puffin/rendering/vulkan/rendersubsystemvk.h"
#include "puffin/rendering/vulkan/resourcemanagervk.h"
#include "puffin/rendering/vulkan/unifiedgeometrybuffervk.h"
#include "puffin/rendering/vulkan/rendergraph/rendergraphvk.h"
#include "puffin/rendering/vulkan/rendergraph/renderpassvk.h"
#include "puffin/rendering/vulkan/rendermodule/corerendermodulevk.h"

namespace puffin::rendering
{
	Forward3DRenderModuleVK::Forward3DRenderModuleVK(std::shared_ptr<core::Engine> engine, RenderSubsystemVK* renderSubsystem)
		: RenderModuleVK(engine, renderSubsystem)
	{
	}

	void Forward3DRenderModuleVK::RegisterModules()
	{
		mRenderSubsystem->RegisterModule<CoreRenderModuleVK>("Core");
	}

	void Forward3DRenderModuleVK::Initialize()
	{
		mColorFormat = vk::Format::eR8G8B8A8Unorm;
		mDepthFormat = vk::Format::eD32Sfloat;

		InitAttachments();
	}

	void Forward3DRenderModuleVK::Deinitialize()
	{
		const auto resourceManager = mRenderSubsystem->GetResourceManager();

		mDefaultForwardPipeline = {};
		mForwardPipelineLayout = {};

		resourceManager->DestroyResource(mColorResourceID);
		resourceManager->DestroyResource(mDepthResourceID);

		mColorResourceID = gInvalidID;
		mDepthResourceID = gInvalidID;
	}

	void Forward3DRenderModuleVK::PostInitialize()
	{
		InitPipelineLayout();
		InitDefaultForwardPipeline();
	}

	void Forward3DRenderModuleVK::UpdateResources(ResourceManagerVK* resourceManager)
	{

	}

	void Forward3DRenderModuleVK::UpdateGraph(RenderGraphVK& renderGraph)
	{
		auto& forwardPass = renderGraph.AddRenderPass("forward3d", RenderPassType::Graphics);
		forwardPass.AddOutputColorAttachment("forward3d_color");
		forwardPass.SetOutputDepthStencilAttachment("forward3d_depth");

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
		const auto unifiedGeometryBuffer = mRenderSubsystem->GetUnifiedGeometryBuffer();
		const auto materialRegistry = mRenderSubsystem->GetMaterialRegistry();
		const auto resourceManager = mRenderSubsystem->GetResourceManager();

		const vk::Extent2D& renderExtent = mRenderSubsystem->GetRenderExtent();
		
		AllocatedImage& color = resourceManager->GetImage(mColorResourceID);
		AllocatedImage& depth = resourceManager->GetImage(mDepthResourceID);
		AllocatedBuffer& indirectBuffer = resourceManager->GetBuffer("indirect_draw");		

		const auto coreRenderModule = mRenderSubsystem->GetModule<CoreRenderModuleVK>("Core");
		const CoreRenderModuleVK::FrameRenderData& frameRenderData = coreRenderModule->GetCurrentFrameData();
		const std::vector<MeshDrawBatch>& meshDrawBatches = coreRenderModule->GetMeshDrawBatches();

		const auto cameraSubsystem = mEngine->GetSubsystem<CameraSubsystem>();
		const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		auto registry = enttSubsystem->GetRegistry();
		
		vk::CommandBufferBeginInfo cmd_begin_info = {
			vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
			nullptr, nullptr
		};

		util::CheckResult(cmd.begin(&cmd_begin_info));
		
		//	1. Transition to Color Attachment Optimal

		vk::ImageSubresourceRange imageSubresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};

		vk::ImageMemoryBarrier offscreenMemoryBarrierToColor = {
			vk::AccessFlagBits::eNone, vk::AccessFlagBits::eColorAttachmentWrite,
			vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal, {}, {},
			color.image, imageSubresourceRange
		};

		cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eColorAttachmentOutput,
							{}, 0, nullptr, 0, nullptr,
							1, &offscreenMemoryBarrierToColor);

		//	2. Begin Rendering

		vk::ClearValue colorClear;
		colorClear.color = {0.0f, 0.7f, 0.9f, 1.0f};

		vk::ClearValue depthClear;
		depthClear.depthStencil.depth = 1.f;

		vk::RenderingAttachmentInfoKHR colorAttachInfo = {
			color.imageView, vk::ImageLayout::eColorAttachmentOptimal, vk::ResolveModeFlagBits::eNone, {},
			vk::ImageLayout::eUndefined, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, colorClear
		};

		vk::RenderingAttachmentInfoKHR depthAttachInfo = {
			depth.imageView, vk::ImageLayout::eDepthStencilAttachmentOptimal, vk::ResolveModeFlagBits::eNone, {},
			vk::ImageLayout::eUndefined, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, depthClear
		};

		vk::RenderingInfoKHR renderInfo = {
			{}, vk::Rect2D{{0, 0}, renderExtent}, 1, {}, 1, &colorAttachInfo, &depthAttachInfo
		};

		cmd.beginRendering(&renderInfo);

		//	3. Set Draw Parameters (Viewport, Scissor, etc...)

		vk::Viewport viewport = {
			0, 0, static_cast<float>(renderExtent.width), static_cast<float>(renderExtent.height), 0.1f, 1.0f
		};
		cmd.setViewport(0, 1, &viewport);

		vk::Rect2D scissor = { {0, 0}, {renderExtent.width, renderExtent.height} };
		cmd.setScissor(0, 1, &scissor);

		//	4. Bind Buffers & Descriptors

		std::vector<vk::DescriptorSet> descriptors =
		{
			frameRenderData.objectDescriptor,
			frameRenderData.lightDescriptor,
			frameRenderData.materialDescriptor,
			frameRenderData.shadowDescriptor,
		};

		cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, mForwardPipelineLayout.get(), 0, descriptors.size(),
			descriptors.data(), 0, nullptr);

		auto entity = enttSubsystem->GetEntity(cameraSubsystem->GetActiveCameraID());
		const auto& camera = registry->get<CameraComponent3D>(entity);
		
		GPUVertexShaderPushConstant pushConstantVert;
		pushConstantVert.vertexBufferAddress = unifiedGeometryBuffer->GetVertexBufferAddress();
		pushConstantVert.camViewProj = camera.viewProj;

		cmd.pushConstants(mForwardPipelineLayout.get(), vk::ShaderStageFlagBits::eVertex, 0, sizeof(GPUVertexShaderPushConstant), &pushConstantVert);
		cmd.pushConstants(mForwardPipelineLayout.get(), vk::ShaderStageFlagBits::eFragment, sizeof(GPUVertexShaderPushConstant), sizeof(GPUFragShaderPushConstant), &frameRenderData.pushConstantFrag);
		
		cmd.bindIndexBuffer(unifiedGeometryBuffer->GetIndexBuffer().buffer, 0, vk::IndexType::eUint32);

		//	5. Render Mesh Batches

		for (const auto& meshDrawBatch : meshDrawBatches)
		{
			//		5a. Bind Pipeline or Batch

			// Use loaded material if id is valid, otherwise use default material
			if (meshDrawBatch.matID != gInvalidID)
			{
				cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, materialRegistry->GetMaterial(meshDrawBatch.matID).pipeline.get());
			}
			else
			{
				cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, mDefaultForwardPipeline.get());
			}

			//		5b. Draw Batch

			vk::DeviceSize indirectOffset = meshDrawBatch.cmdIndex * sizeof(vk::DrawIndexedIndirectCommand);
			uint32_t drawStride = sizeof(vk::DrawIndexedIndirectCommand);

			RenderSubsystemVK::DrawIndirectCommandParams params;
			params.buffer = indirectBuffer.buffer;
			params.offset = indirectOffset;
			params.drawCount = meshDrawBatch.cmdCount;
			params.stride = drawStride;

			mRenderSubsystem->DrawIndexedIndirectCommand(cmd, params);
		}

		//	5. End Rendering

		cmd.endRendering();

		//	6. Transition to Shader Read Optimal

		vk::ImageMemoryBarrier offscreenMemoryBarrierToShader = {
			vk::AccessFlagBits::eColorAttachmentWrite, vk::AccessFlagBits::eNone,
			vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, {}, {},
			color.image, imageSubresourceRange
		};

		cmd.pipelineBarrier(vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eBottomOfPipe,
							{}, 0, nullptr, 0, nullptr,
							1, &offscreenMemoryBarrierToShader);

		cmd.end();
	}

	void Forward3DRenderModuleVK::InitAttachments()
	{
		auto resourceManager = mRenderSubsystem->GetResourceManager();

		AttachmentDescVK color;
		color.format = mColorFormat;
		color.type = AttachmentTypeVK::Color;

		mColorResourceID = resourceManager->CreateOrUpdateAttachment(color, "forward3d_color");

		AttachmentDescVK depth;
		depth.format = mDepthFormat;
		depth.type = AttachmentTypeVK::Depth;

		mDepthResourceID = resourceManager->CreateOrUpdateAttachment(depth, "forward3d_depth");
	}

	void Forward3DRenderModuleVK::InitPipelineLayout()
	{
		auto resourceManager = mRenderSubsystem->GetResourceManager();
		const vk::DescriptorSetLayout& objectLayout = resourceManager->GetDescriptorLayout("objects");
		const vk::DescriptorSetLayout& lightLayout = resourceManager->GetDescriptorLayout("lights");
		const vk::DescriptorSetLayout& materialLayout = resourceManager->GetDescriptorLayout("materials");
		const vk::DescriptorSetLayout& shadowLayout = resourceManager->GetDescriptorLayout("shadows");

		constexpr vk::PushConstantRange vertRange = { vk::ShaderStageFlagBits::eVertex, 0, sizeof(GPUVertexShaderPushConstant) };
		constexpr vk::PushConstantRange fragRange = { vk::ShaderStageFlagBits::eFragment, sizeof(GPUVertexShaderPushConstant), sizeof(GPUFragShaderPushConstant) };

		util::PipelineLayoutBuilder plb{};
		mForwardPipelineLayout = plb
			.DescriptorSetLayout(objectLayout)
			.DescriptorSetLayout(lightLayout)
			.DescriptorSetLayout(materialLayout)
			.DescriptorSetLayout(shadowLayout)
			.PushConstantRange(vertRange)
			.PushConstantRange(fragRange)
			.CreateUnique(mRenderSubsystem->GetDevice());
	}

	void Forward3DRenderModuleVK::InitDefaultForwardPipeline()
	{
		const vk::Device& device = mRenderSubsystem->GetDevice();

		mDefaultForwardVertMod = util::ShaderModule{
			device, fs::path(assets::AssetRegistry::Get()->GetEngineRoot() / "bin" / "vulkan" / "forward_shading" / "forward_shading_vs.spv").string()
		};

		mDefaultForwardFragMod = util::ShaderModule{
			device, fs::path(assets::AssetRegistry::Get()->GetEngineRoot() / "bin" / "vulkan" / "forward_shading" / "forward_shading_fs.spv").string()
		};

		vk::PipelineDepthStencilStateCreateInfo depthStencilInfo = {
			{}, true, true,
			vk::CompareOp::eLessOrEqual, false, false, {}, {}, 0.0f, 1.0f
		};

		vk::PipelineRenderingCreateInfoKHR pipelineRenderInfo = {
			0, mColorFormat, mDepthFormat
		};

		util::PipelineBuilder pb{ 1280, 720 };
		mDefaultForwardPipeline = pb
			// Define dynamic state which can change each frame (currently viewport and scissor size)
			.DynamicState(vk::DynamicState::eViewport)
			.DynamicState(vk::DynamicState::eScissor)
			// Define vertex/fragment shaders
			.Shader(vk::ShaderStageFlagBits::eVertex, mDefaultForwardVertMod)
			.Shader(vk::ShaderStageFlagBits::eFragment, mDefaultForwardFragMod)
			.DepthStencilState(depthStencilInfo)
			// Add rendering info struct
			.AddPNext(&pipelineRenderInfo)
			// Create pipeline
			.CreateUnique(device, mRenderSubsystem->GetPipelineCache(), *mForwardPipelineLayout, nullptr);

		device.destroyShaderModule(mDefaultForwardVertMod.Module());
		device.destroyShaderModule(mDefaultForwardFragMod.Module());
	}
}
