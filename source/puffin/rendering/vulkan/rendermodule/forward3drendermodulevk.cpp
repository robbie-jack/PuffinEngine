#include "puffin/rendering/vulkan/rendermodule/forward3drendermodulevk.h"

#include "puffin/assets/assetregistry.h"
#include "puffin/rendering/rendersubsystem.h"
#include "puffin/rendering/vulkan/rendersubsystemvk.h"
#include "puffin/rendering/vulkan/resourcemanagervk.h"
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
		auto resourceManager = mRenderSubsystem->GetResourceManager();

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
		//	1. Begin Rendering



		//	2. Set Draw Parameters (Viewport, Scissor, etc...)



		//	3. Bind Buffers & Descriptors



		//	4. Render Mesh Batches

		//		4a. Bind Pipeline or Batch



		//		4b. Draw Batch



		//	5. End Rendering


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
