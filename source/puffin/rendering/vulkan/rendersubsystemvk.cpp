#include "puffin/rendering/vulkan/rendersubsystemvk.h"

#include <iostream>

#define VMA_IMPLEMENTATION
#define VMA_DEBUG_LOG

// If you don't like the `vma::` prefix:
//#define VMA_HPP_NAMESPACE <prefix>

#include "GLFW/glfw3.h"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/rotate_vector.hpp>

#include "puffin/mathhelpers.h"
#include "VkBootstrap.h"
#include "vk_mem_alloc.h"
#include "vk_mem_alloc.hpp"

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"

#include "puffin/core/engine.h"
#include "puffin/rendering/vulkan/helpersvk.h"
#include "puffin/core/enkitssubsystem.h"
#include "puffin/components/transformcomponent2d.h"
#include "puffin/components/transformcomponent3d.h"
#include "puffin/ecs/enttsubsystem.h"
#include "puffin/input/inputsubsystem.h"
#include "puffin/assets/assetregistry.h"
#include "puffin/components/physics/2d/velocitycomponent2d.h"
#include "puffin/components/physics/3d/velocitycomponent3d.h"
#include "puffin/components/rendering/3d/directionallightcomponent3d.h"
#include "puffin/components/rendering/3d/pointlightcomponent3d.h"
#include "puffin/components/rendering/3d/shadowcastercomponent3d.h"
#include "puffin/components/rendering/3d/spotlightcomponent3d.h"
#include "puffin/components/rendering/3d/staticmeshcomponent3d.h"
#include "puffin/components/rendering/3d/cameracomponent3d.h"
#include "puffin/core/settingsmanager.h"
#include "puffin/core/signalsubsystem.h"
#include "puffin/rendering/camerasubsystem.h"
#include "puffin/rendering/materialglobals.h"
#include "puffin/window/windowsubsystem.h"
#include "puffin/scene/scenegraphsubsystem.h"
#include "puffin/editor/ui/editoruisubsystem.h"
#include "puffin/rendering/vulkan/resourcemanagervk.h"
#include "puffin/rendering/vulkan/materialregistryvk.h"
#include "puffin/rendering/vulkan/unifiedgeometrybuffervk.h"
#include "puffin/editor/ui/windows/uiwindowviewport.h"
#include "puffin/nodes/transformnode2d.h"
#include "puffin/nodes/transformnode3d.h"
#include "puffin/rendering/renderglobals.h"
#include "puffin/rendering/renderhelpers.h"
#include "puffin/rendering/vulkan/rendermodule/forward3drendermodulevk.h"
#include "puffin/rendering/vulkan/rendermodule/rendermodulevk.h"

#define VK_CHECK(x)                                                 \
	do                                                              \
	{                                                               \
		vk::Result err = x;                                         \
		if (err != vk::Result::eSuccess)                            \
		{                                                           \
			std::cout <<"Detected Vulkan error: " << err << std::endl; \
			abort();                                                \
		}                                                           \
	} while (0)

namespace puffin::rendering
{
	RenderSubsystemVK::RenderSubsystemVK(const std::shared_ptr<core::Engine>& engine) : RenderSubsystem(engine)
	{
		mName = "RenderSubystemVK";
	}

	RenderSubsystemVK::~RenderSubsystemVK()
	{
	}

	void RenderSubsystemVK::Initialize(core::SubsystemManager* subsystemManager)
	{
		const auto windowSubsystem = subsystemManager->CreateAndInitializeSubsystem<window::WindowSubsystem>();
		const auto enttSubsystem = subsystemManager->CreateAndInitializeSubsystem<ecs::EnTTSubsystem>();

		// Bind callbacks
		const auto registry = enttSubsystem->GetRegistry();

		registry->on_construct<StaticMeshComponent3D>().connect<&RenderSubsystemVK::OnUpdateMesh>(this);
		registry->on_update<StaticMeshComponent3D>().connect<&RenderSubsystemVK::OnUpdateMesh>(this);
		registry->on_destroy<StaticMeshComponent3D>().connect<&RenderSubsystemVK::OnDestroyMeshOrTransform>(this);

		registry->on_construct<TransformComponent2D>().connect<&RenderSubsystemVK::OnUpdateTransform>(this);
		registry->on_update<TransformComponent2D>().connect<&RenderSubsystemVK::OnUpdateTransform>(this);
		registry->on_destroy<TransformComponent2D>().connect<&RenderSubsystemVK::OnDestroyMeshOrTransform>(this);

		registry->on_construct<TransformComponent3D>().connect<&RenderSubsystemVK::OnUpdateTransform>(this);
		registry->on_update<TransformComponent3D>().connect<&RenderSubsystemVK::OnUpdateTransform>(this);
		registry->on_destroy<TransformComponent3D>().connect<&RenderSubsystemVK::OnDestroyMeshOrTransform>(this);

		registry->on_construct<ShadowCasterComponent3D>().connect<&RenderSubsystemVK::OnConstructOrUpdateShadowCaster>(this);
		registry->on_update<ShadowCasterComponent3D>().connect<&RenderSubsystemVK::OnConstructOrUpdateShadowCaster>(this);
		registry->on_destroy<ShadowCasterComponent3D>().connect<&RenderSubsystemVK::OnDestroyShadowCaster>(this);

		const auto signalSubsystem = mEngine->GetSubsystem<core::SignalSubsystem>();
		auto renderingDrawShadowsSignal = signalSubsystem->GetSignal<bool>("rendering_draw_shadows");
		if (!renderingDrawShadowsSignal)
		{
			renderingDrawShadowsSignal = signalSubsystem->CreateSignal<bool>("rendering_draw_shadows");
		}

		renderingDrawShadowsSignal->Connect(std::function([&](const bool& renderingDrawShadows)
		{
			mRenderShadows = renderingDrawShadows;
		}));

		// Initialise vulkan and all rendering objects
		InitVulkan();

		mResourceManager = std::make_unique<ResourceManagerVK>(this, gBufferedFrameCount);
		mUnifiedGeometryBuffer = std::make_unique<UnifiedGeometryBufferVK>(this);

		InitSwapchain(mSwapchainData, mSwapchainDataOld.swapchain, mSwapchainExtent);

		if (mEngine->GetShouldRenderEditorUI())
		{
			const auto editorUISubsystem = subsystemManager->CreateAndInitializeSubsystem<ui::EditorUISubsystem>();
			const ImVec2 viewportSize = editorUISubsystem->GetWindowViewport()->GetViewportSize();
			mRenderExtent.width = viewportSize.x;
			mRenderExtent.height = viewportSize.y;
		}
		else
		{
			mRenderExtent = mSwapchainExtent;
		}

		InitOffscreen(mOffscreenData, mRenderExtent, mSwapchainData.images.size());

		RegisterModule<Forward3DRenderModuleVK>("Forward3D");

		InitModules();

		mResourceManager->CreateAndUpdateResources();

		InitCommands();

		InitSyncStructures();
		InitBuffers();
		InitSamplers();

		BuildDescriptors();
		InitPipelines();

		if (mEngine->GetShouldRenderEditorUI())
		{
			InitImgui();
			InitOffscreenImguiTextures(mOffscreenData);
		}

		mRenderables.reserve(gMaxObjects);
		mCachedObjectData.Reserve(gMaxObjects);

		mMaterialRegistry = std::make_unique<MaterialRegistryVK>(this);

		mInitialized = true;
		mUpdateRenderables = true;

		const auto settingsManager = mEngine->GetSubsystem<core::SettingsManager>();
		mRenderShadows = settingsManager->Get<bool>("rendering_draw_shadows");
	}

	void RenderSubsystemVK::Deinitialize()
	{
		mDevice.waitIdle();

		if (mInitialized)
		{
			for (auto texData : mTexData)
			{
				UnloadTexture(texData);
			}

			mTexData.Clear();

			CleanOffscreen(mOffscreenData);

			if (mOffscreenDataOld.needsCleaned)
			{
				CleanOffscreen(mOffscreenDataOld);
			}

			DeinitModules();

			mResourceManager->DestroyResources();

			//m_material_registry = nullptr;
			mUnifiedGeometryBuffer = nullptr;
			mResourceManager = nullptr;

			CleanSwapchain(mSwapchainData);

			if (mSwapchainDataOld.needsCleaned)
			{
				CleanSwapchain(mSwapchainDataOld);
			}

			for (auto& frameData : mFrameRenderData)
			{
				for (auto& [name, semaphore] : frameData.semaphores)
				{
					mDevice.destroySemaphore(semaphore);
				}

				frameData.semaphores.clear();
			}

			mDeletionQueue.Flush();

			mRenderGraph.Reset();

			mInitialized = false;
		}
	}

	core::SubsystemType RenderSubsystemVK::GetType() const
	{
		return core::SubsystemType::Render;
	}

	double RenderSubsystemVK::WaitForLastPresentationAndSampleTime()
	{
		// Wait until GPU has finished presenting last frame. Timeout of 1 second
		VK_CHECK(mDevice.waitForFences(1, &GetCurrentFrameData().presentFence, true, 1000000000));
		VK_CHECK(mDevice.resetFences(1, &GetCurrentFrameData().presentFence));

		VK_CHECK(mDevice.acquireNextImageKHR(mSwapchainData.swapchain, 1000000000, GetCurrentFrameData().presentSemaphore,
				GetCurrentFrameData().presentFence, &mCurrentSwapchainIdx));

		return glfwGetTime();
	}

	void RenderSubsystemVK::Render(double deltaTime)
	{
		//	1. Wait for nth frame to finish rendering (N - 0 to number of Frames in Flight)
		WaitForRenderFence();

		//	2. Update Render Resources (Buffers, Images, Descriptions Sync Structures, etc...)
		UpdateShadows();
		UpdateResources();

		//	3. Reset & Build Graph
		UpdateGraph();

		//	4. Pre-Render - Upload all needed data to GPU (Textures, Object Data, Materials, etc...)

		PreRender(deltaTime);

		UpdateRenderData();

		ProcessComponents();

		UpdateSwapchainAndOffscreen();

		RenderEditorUI();

		UpdateTextureDescriptors();

		UpdateShadowDescriptors();

		PrepareSceneData();

		BuildIndirectCommands();

		//	5. Render - Record & Submit Render Commands

		mDrawCallsCountTotal = 0;
		mDrawCallsCount.clear();

		//		5a. Record Resource Transition Commands - PUFFIN_TODO



		//		5b. Record Render Module Commands

		RecordRenderPassCommands();

		//		5c. Submit Commands

		RecordAndSubmitCommands(mCurrentSwapchainIdx);

		//	6. Destroy Old Resources
		mResourceManager->DestroyResources();

		mFrameCount++;
	}

	void RenderSubsystemVK::OnUpdateMesh(entt::registry& registry, entt::entity entity)
	{
		const auto mesh = registry.get<StaticMeshComponent3D>(entity);

		if (mesh.meshID == gInvalidID || mesh.materialID == gInvalidID)
		{
			return;
		}

		mMeshesToLoad.insert(mesh.meshID);
		mMaterialRegistry->RegisterMaterialInstance(mesh.materialID);

		AddRenderable(registry, entity);
	}

	void RenderSubsystemVK::OnUpdateTransform(entt::registry& registry, entt::entity entity)
	{
		AddRenderable(registry, entity);
	}

	void RenderSubsystemVK::OnDestroyMeshOrTransform(entt::registry& registry, entt::entity entity)
	{
		mUpdateRenderables = true;
	}

	void RenderSubsystemVK::AddRenderable(entt::registry& registry, entt::entity entity)
	{
		if (registry.any_of<TransformComponent2D, TransformComponent3D>(entity) && registry.any_of<StaticMeshComponent3D>(entity))
		{
			const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
			const auto id = enttSubsystem->GetID(entity);
			const auto mesh = registry.get<StaticMeshComponent3D>(entity);

			if (mesh.meshID == gInvalidID || mesh.materialID == gInvalidID)
			{
				return;
			}

			mObjectsToRefresh.insert(id);

			mUpdateRenderables = true;
		}
	}

	void RenderSubsystemVK::OnConstructOrUpdateShadowCaster(entt::registry& registry, entt::entity entity)
	{
		const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		const auto id = enttSubsystem->GetID(entity);

		mShadowsToUpdate.emplace(id);
	}

	void RenderSubsystemVK::OnDestroyShadowCaster(entt::registry& registry, entt::entity entity)
	{
		const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		const auto id = enttSubsystem->GetID(entity);
		auto& shadow = registry.get<ShadowCasterComponent3D>(entity);

		for (const auto& resourceID : shadow.shadowCascadeIDs)
		{
			mShadowResourcesToDestroy.emplace(resourceID);
		}

		shadow.shadowCascadeIDs.clear();
	}

	void RenderSubsystemVK::RegisterTexture(UUID textureID)
	{
		mTexturesToLoad.insert(textureID);
	}

	void RenderSubsystemVK::DrawCommand(vk::CommandBuffer& cmd, const DrawCommandParams& cmdParams)
	{
		cmd.draw(cmdParams.vertexCount, cmdParams.instanceCount, cmdParams.firstVertex, cmdParams.firstInstance);

		mDrawCallsCountTotal++;

		if (mDrawCallsCount.find("Draw") == mDrawCallsCount.end())
		{
			mDrawCallsCount.emplace("Draw", 0);
		}

		mDrawCallsCount["Draw"]++;
	}

	void RenderSubsystemVK::DrawIndexedCommand(vk::CommandBuffer& cmd, const DrawIndexedCommandParams& cmdParams)
	{
		cmd.drawIndexed(cmdParams.indexCount, cmdParams.instanceCount, cmdParams.firstIndex, cmdParams.vertexOffset, cmdParams.firstInstance);

		mDrawCallsCountTotal++;

		if (mDrawCallsCount.find("Draw") == mDrawCallsCount.end())
		{
			mDrawCallsCount.emplace("Draw", 0);
		}

		mDrawCallsCount["Draw"]++;
	}

	void RenderSubsystemVK::DrawIndirectCommand(vk::CommandBuffer& cmd, const DrawIndirectCommandParams& cmdParams)
	{
		cmd.drawIndirect(cmdParams.buffer, cmdParams.offset, cmdParams.drawCount, cmdParams.stride);

		mDrawCallsCountTotal++;

		if (mDrawCallsCount.find("DrawIndirect") == mDrawCallsCount.end())
		{
			mDrawCallsCount.emplace("DrawIndirect", 0);
		}

		mDrawCallsCount["DrawIndirect"]++;
	}

	void RenderSubsystemVK::DrawIndexedIndirectCommand(vk::CommandBuffer& cmd,
		const DrawIndirectCommandParams& cmdParams)
	{
		cmd.drawIndexedIndirect(cmdParams.buffer, cmdParams.offset, cmdParams.drawCount, cmdParams.stride);

		mDrawCallsCountTotal++;

		if (mDrawCallsCount.find("DrawIndirect") == mDrawCallsCount.end())
		{
			mDrawCallsCount.emplace("DrawIndirect", 0);
		}

		mDrawCallsCount["DrawIndirect"]++;
	}

	void RenderSubsystemVK::InitVulkan()
	{
		auto windowSubsystem = mEngine->GetSubsystem<window::WindowSubsystem>();
		GLFWwindow* glfwWindow = windowSubsystem->GetPrimaryWindow();

		glfwSetWindowUserPointer(glfwWindow, this);
		glfwSetFramebufferSizeCallback(glfwWindow, FrameBufferResizeCallback);

		// Create Vulkan Instance
		int width, height;
		glfwGetWindowSize(glfwWindow, &width, &height);
		mSwapchainExtent.width = static_cast<unsigned>(width);
		mSwapchainExtent.height = static_cast<unsigned>(height);

		vkb::InstanceBuilder instBuilder;

		auto instRet = instBuilder.set_app_name("Puffin Engine")
		.request_validation_layers(gEnableValidationLayers)
		.require_api_version(1, 3, 25)
		.use_default_debug_messenger()
		.enable_extension("VK_KHR_get_physical_device_properties2")
		.build();

		vkb::Instance vkbInst = instRet.value();

		// Store Instance/Debug Messenger
		mInstance = vkbInst.instance;
		mDebugMessenger = vkbInst.debug_messenger;

		// Get Window Surface
		VkSurfaceKHR surface;
		glfwCreateWindowSurface(mInstance, glfwWindow, nullptr, &surface);
		mSurface = surface;

		vk::PhysicalDeviceFeatures physicalDeviceFeatures = {};
		physicalDeviceFeatures.shaderSampledImageArrayDynamicIndexing = true;
		physicalDeviceFeatures.drawIndirectFirstInstance = true;
		physicalDeviceFeatures.multiDrawIndirect = true;
		physicalDeviceFeatures.samplerAnisotropy = true;

		vk::PhysicalDeviceVulkan12Features physicalDevice12Features = {};
		physicalDevice12Features.descriptorIndexing = true;
		physicalDevice12Features.runtimeDescriptorArray = true;
		physicalDevice12Features.descriptorBindingVariableDescriptorCount = true;
		physicalDevice12Features.descriptorBindingPartiallyBound = true;
		physicalDevice12Features.bufferDeviceAddress = true;

		vk::PhysicalDeviceVulkan13Features physicalDevice13Features = {};
		physicalDevice13Features.dynamicRendering = true;

		vkb::SystemInfo systemInfo = vkb::SystemInfo::get_system_info().value();

		// Check for desired extension support
		std::vector<const char*> deviceExtensions =
		{
			"VK_EXT_memory_budget",
		};

		// Select GPU
		vkb::PhysicalDeviceSelector selector{vkbInst};
		vkb::PhysicalDevice physDevice = selector
		.set_minimum_version(1, 3)
		.set_surface(mSurface)
		.set_required_features(physicalDeviceFeatures)
		.set_required_features_12(physicalDevice12Features)
		.set_required_features_13(physicalDevice13Features)
		.add_required_extensions(deviceExtensions)
		.select()
		.value();

		// Create Vulkan Device
		vkb::DeviceBuilder deviceBuilder{physDevice};

		vk::PhysicalDeviceShaderDrawParametersFeatures shaderDrawParametersFeatures = {true};

		vkb::Device vkbDevice = deviceBuilder
		.add_pNext(&shaderDrawParametersFeatures)
		.build()
		.value();

		mDevice = vkbDevice.device;
		mPhysicalDevice = physDevice.physical_device;

		// Get Graphics Queue
		mGraphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
		mGraphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

		// Init memory allocator
		vma::AllocatorCreateInfo allocatorInfo = {vma::AllocatorCreateFlagBits::eBufferDeviceAddress, mPhysicalDevice, mDevice};
		allocatorInfo.instance = mInstance;

		VK_CHECK(vma::createAllocator(&allocatorInfo, &mAllocator));

		// Check memory types
		const vk::PhysicalDeviceMemoryProperties* deviceMemProps = mAllocator.getMemoryProperties();

		for (int i = 0; i < deviceMemProps->memoryTypeCount; i++)
		{
			const vk::MemoryType& memType = deviceMemProps->memoryTypes[i];
			const vk::MemoryPropertyFlags& memPropFlags = memType.propertyFlags;

			if (memPropFlags & (vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eDeviceLocal))
			{
				vk::DeviceSize heapSize = deviceMemProps->memoryHeaps[memType.heapIndex].size;
				vk::DeviceSize heapSizeMB = heapSize / 1024 / 1024;

				if (heapSizeMB > 256)
				{
					mRebarEnabled = true;
				}
			}
		}

		mDeletionQueue.PushFunction([=]()
		{
			mAllocator.destroy();

			mDevice.destroy();
			mInstance.destroySurfaceKHR(mSurface);
			vkb::destroy_debug_utils_messenger(mInstance, mDebugMessenger);
			mInstance.destroy();
		});
	}

	void RenderSubsystemVK::InitSwapchain(SwapchainData& swapchainData, vk::SwapchainKHR& oldSwapchain,
	                                      const vk::Extent2D& swapchainExtent)
	{
		vkb::SwapchainBuilder swapchainBuilder{mPhysicalDevice, mDevice, mSurface};

		swapchainData.extent = swapchainExtent;

		vkb::Swapchain vkbSwapchain = swapchainBuilder
		.use_default_format_selection()
		// Vsync present mode
		.set_old_swapchain(oldSwapchain)
		.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
		.set_desired_extent(swapchainData.extent.width, swapchainData.extent.height)
		.set_image_usage_flags(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT)
		.build()
		.value();

		swapchainData.swapchain = vkbSwapchain.swapchain;
		swapchainData.imageFormat = static_cast<vk::Format>(vkbSwapchain.image_format);

		// Grab Images
		std::vector<VkImage> images = vkbSwapchain.get_images().value();
		std::vector<VkImageView> imageViews = vkbSwapchain.get_image_views().value();

		swapchainData.images.resize(images.size());
		swapchainData.imageViews.resize(imageViews.size());

		for (int i = 0; i < images.size(); i++)
		{
			swapchainData.images[i] = static_cast<vk::Image>(images[i]);
			swapchainData.imageViews[i] = static_cast<vk::ImageView>(imageViews[i]);
		}

		images.clear();
		imageViews.clear();
	}

	void RenderSubsystemVK::InitModules()
	{
		for (auto renderModule : mRenderModuleVector)
		{
			renderModule->Initialize();
		}
	}

	void RenderSubsystemVK::InitOffscreen(OffscreenData& offscreenData, const vk::Extent2D& offscreenExtent,
	                                   const int& offscreenImageCount)
	{
		offscreenData.extent = offscreenExtent;

		const vk::Extent3D imageExtent = {offscreenData.extent.width, offscreenData.extent.height, 1};

		offscreenData.imageFormat = vk::Format::eR8G8B8A8Unorm;

		util::CreateImageParams createImageParams;

		createImageParams.imageInfo = {
			{}, vk::ImageType::e2D, offscreenData.imageFormat, imageExtent,
			1, 1, vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
			{
				vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled |
				vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst
			}
		};

		const vk::ImageSubresourceRange subresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};

		createImageParams.imageViewInfo = {
			{}, {}, vk::ImageViewType::e2D, offscreenData.imageFormat, {}, subresourceRange
		};

		offscreenData.allocImages.resize(offscreenImageCount);
		for (int i = 0; i < offscreenImageCount; i++)
		{
			offscreenData.allocImages[i] = util::CreateImage(this, createImageParams);
		}

		util::CreateFormattedImageParams params;
		params.extent = imageExtent;
		params.format = vk::Format::eD32Sfloat;

		offscreenData.allocDepthImage = util::CreateDepthImage(this, params);
	}

	void RenderSubsystemVK::InitCommands()
	{
		vk::CommandPoolCreateInfo commandPoolInfo = {
			vk::CommandPoolCreateFlagBits::eResetCommandBuffer, mGraphicsQueueFamily
		};
		vk::CommandBufferAllocateInfo commandBufferInfo = {{}, vk::CommandBufferLevel::ePrimary, 1};

		// Init Main Command Pools/Buffers
		for (int i = 0; i < gBufferedFrameCount; i++)
		{
			VK_CHECK(mDevice.createCommandPool(&commandPoolInfo, nullptr, &mFrameRenderData[i].commandPool));

			commandBufferInfo.commandPool = mFrameRenderData[i].commandPool;
			VK_CHECK(mDevice.allocateCommandBuffers(&commandBufferInfo, &mFrameRenderData[i].shadowCommandBuffer));
			VK_CHECK(mDevice.allocateCommandBuffers(&commandBufferInfo, &mFrameRenderData[i].mainCommandBuffer));
			VK_CHECK(mDevice.allocateCommandBuffers(&commandBufferInfo, &mFrameRenderData[i].copyCommandBuffer));
			VK_CHECK(mDevice.allocateCommandBuffers(&commandBufferInfo, &mFrameRenderData[i].imguiCommandBuffer));

			mDeletionQueue.PushFunction([=]()
			{
				mDevice.destroyCommandPool(mFrameRenderData[i].commandPool);
			});
		}

		// Init Upload Context Command Pool/Buffer
		commandPoolInfo = {vk::CommandPoolCreateFlagBits::eResetCommandBuffer, mGraphicsQueueFamily};
		VK_CHECK(mDevice.createCommandPool(&commandPoolInfo, nullptr, &mUploadContext.commandPool));

		commandBufferInfo = {mUploadContext.commandPool, vk::CommandBufferLevel::ePrimary, 1};
		VK_CHECK(mDevice.allocateCommandBuffers(&commandBufferInfo, &mUploadContext.commandBuffer));

		mDeletionQueue.PushFunction([=]()
		{
			mDevice.destroyCommandPool(mUploadContext.commandPool);
		});
	}

	void RenderSubsystemVK::InitSyncStructures()
	{
		vk::FenceCreateInfo fenceCreateInfo = {vk::FenceCreateFlagBits::eSignaled, nullptr};
		vk::SemaphoreCreateInfo semaphoreCreateInfo = {{}, nullptr};

		for (int i = 0; i < gBufferedFrameCount; i++)
		{
			VK_CHECK(mDevice.createFence(&fenceCreateInfo, nullptr, &mFrameRenderData[i].renderFence));
			VK_CHECK(mDevice.createFence(&fenceCreateInfo, nullptr, &mFrameRenderData[i].presentFence));

			VK_CHECK(mDevice.createSemaphore(&semaphoreCreateInfo, nullptr, &mFrameRenderData[i].renderSemaphore));
			VK_CHECK(mDevice.createSemaphore(&semaphoreCreateInfo, nullptr, &mFrameRenderData[i].copySemaphore));
			VK_CHECK(mDevice.createSemaphore(&semaphoreCreateInfo, nullptr, &mFrameRenderData[i].imguiSemaphore));
			VK_CHECK(mDevice.createSemaphore(&semaphoreCreateInfo, nullptr, &mFrameRenderData[i].presentSemaphore));
			VK_CHECK(mDevice.createSemaphore(&semaphoreCreateInfo, nullptr, &mFrameRenderData[i].shadowSemaphore));

			mDeletionQueue.PushFunction([=]()
			{
				mDevice.destroyFence(mFrameRenderData[i].renderFence);
				mDevice.destroyFence(mFrameRenderData[i].presentFence);

				mDevice.destroySemaphore(mFrameRenderData[i].renderSemaphore);
				mDevice.destroySemaphore(mFrameRenderData[i].copySemaphore);
				mDevice.destroySemaphore(mFrameRenderData[i].imguiSemaphore);
				mDevice.destroySemaphore(mFrameRenderData[i].presentSemaphore);
				mDevice.destroySemaphore(mFrameRenderData[i].shadowSemaphore);
			});
		}

		// Init Upload Context Fence
		fenceCreateInfo = vk::FenceCreateInfo{{}, nullptr};
		VK_CHECK(mDevice.createFence(&fenceCreateInfo, nullptr, &mUploadContext.uploadFence));

		mDeletionQueue.PushFunction([=]()
		{
			mDevice.destroyFence(mUploadContext.uploadFence);
		});
	}

	void RenderSubsystemVK::InitBuffers()
	{
		for (int i = 0; i < gBufferedFrameCount; i++)
		{
			// Indirect Buffer
			util::CreateBufferParams params;
			params.allocSize = sizeof(vk::DrawIndexedIndirectCommand) * gMaxObjects;
			params.bufferUsage = vk::BufferUsageFlagBits::eIndirectBuffer | vk::BufferUsageFlagBits::eStorageBuffer |
				vk::BufferUsageFlagBits::eTransferDst;
			params.allocFlags = vma::AllocationCreateFlagBits::eHostAccessSequentialWrite | vma::AllocationCreateFlagBits::eMapped;
			
			mFrameRenderData[i].indirectDrawBuffer = util::CreateBuffer(mAllocator, params);

			// Global Buffers
			params.allocSize = sizeof(GPUCameraData);
			params.bufferUsage = vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferDst;
			mFrameRenderData[i].cameraBuffer = util::CreateBuffer(mAllocator, params);

			params.allocSize = sizeof(GPUPointLightData) * gMaxPointLights;
			params.bufferUsage = vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst;
			mFrameRenderData[i].pointLightBuffer = util::CreateBuffer(mAllocator, params);

			params.allocSize = sizeof(GPUSpotLightData) * gMaxSpotLights;
			mFrameRenderData[i].spotLightBuffer = util::CreateBuffer(mAllocator, params);

			params.allocSize = sizeof(GPUDirLightData) * gMaxDirectionalLights;
			mFrameRenderData[i].directionalLightBuffer = util::CreateBuffer(mAllocator, params);

			params.allocSize = sizeof(GPUShadowData) * gMaxLights;
			mFrameRenderData[i].shadowBuffer = util::CreateBuffer(mAllocator, params);

			params.allocSize = sizeof(GPUShadowCascadeData) * gMaxLights * gMaxShadowCascadesPerLight;
            mFrameRenderData[i].shadowCascadeBuffer = util::CreateBuffer(mAllocator, params);

			params.allocSize = sizeof(GPUObjectData) * gMaxObjects;
			mFrameRenderData[i].objectBuffer = util::CreateBuffer(mAllocator, params);

			params.allocSize = sizeof(GPUMaterialInstanceData) * gMaxMaterialInstances;
			mFrameRenderData[i].materialInstanceBuffer = util::CreateBuffer(mAllocator, params);

			// Material Buffers

			// Object Buffers

			mDeletionQueue.PushFunction([=]()
			{
				mAllocator.destroyBuffer(mFrameRenderData[i].materialInstanceBuffer.buffer,
					mFrameRenderData[i].materialInstanceBuffer.allocation);

				mAllocator.destroyBuffer(mFrameRenderData[i].objectBuffer.buffer,
					mFrameRenderData[i].objectBuffer.allocation);

				mAllocator.destroyBuffer(mFrameRenderData[i].shadowCascadeBuffer.buffer,
					mFrameRenderData[i].shadowCascadeBuffer.allocation);

				mAllocator.destroyBuffer(mFrameRenderData[i].shadowBuffer.buffer,
					mFrameRenderData[i].shadowBuffer.allocation);

				mAllocator.destroyBuffer(mFrameRenderData[i].directionalLightBuffer.buffer,
					mFrameRenderData[i].directionalLightBuffer.allocation);

				mAllocator.destroyBuffer(mFrameRenderData[i].spotLightBuffer.buffer,
					mFrameRenderData[i].spotLightBuffer.allocation);

				mAllocator.destroyBuffer(mFrameRenderData[i].pointLightBuffer.buffer,
					mFrameRenderData[i].pointLightBuffer.allocation);


				mAllocator.destroyBuffer(mFrameRenderData[i].cameraBuffer.buffer,
					mFrameRenderData[i].cameraBuffer.allocation);

				mAllocator.destroyBuffer(mFrameRenderData[i].indirectDrawBuffer.buffer,
					mFrameRenderData[i].indirectDrawBuffer.allocation);
			});
		}
	}

	void RenderSubsystemVK::InitSamplers()
	{
		vk::SamplerCreateInfo textureSamplerInfo = {};
		textureSamplerInfo.anisotropyEnable = true;
		textureSamplerInfo.maxAnisotropy = 16.0f;

		mGlobalRenderData.textureSampler = mDevice.createSampler(textureSamplerInfo);

		vk::SamplerCreateInfo shadowmapSamplerInfo = {};
		shadowmapSamplerInfo.borderColor = vk::BorderColor::eFloatOpaqueWhite;
		shadowmapSamplerInfo.addressModeU = vk::SamplerAddressMode::eClampToBorder;
		shadowmapSamplerInfo.addressModeV = vk::SamplerAddressMode::eClampToBorder;
		shadowmapSamplerInfo.addressModeW = vk::SamplerAddressMode::eClampToBorder;

		mGlobalRenderData.shadowmapSampler = mDevice.createSampler(shadowmapSamplerInfo);

		mDeletionQueue.PushFunction([=]()
		{
			mDevice.destroySampler(mGlobalRenderData.textureSampler, nullptr);
			mDevice.destroySampler(mGlobalRenderData.shadowmapSampler, nullptr);
		});
	}

	void RenderSubsystemVK::BuildDescriptors()
	{
		// Descriptor Allocator/Cache

		mGlobalRenderData.descriptorAllocator = std::make_shared<util::DescriptorAllocator>(mDevice);
		mGlobalRenderData.descriptorLayoutCache = std::make_shared<util::DescriptorLayoutCache>(mDevice);

		for (int i = 0; i < gBufferedFrameCount; i++)
		{
			// Global Descriptors

			vk::DescriptorBufferInfo objectBufferInfo = {
				mFrameRenderData[i].objectBuffer.buffer, 0, sizeof(GPUObjectData) * gMaxObjects
			};

			util::DescriptorBuilder::Begin(mGlobalRenderData.descriptorLayoutCache,
				mGlobalRenderData.descriptorAllocator)
				.BindBuffer(0, &objectBufferInfo, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eVertex)
				.Build(mFrameRenderData[i].objectDescriptor, mGlobalRenderData.objectSetLayout);

			vk::DescriptorBufferInfo cameraBufferInfo = {
				mFrameRenderData[i].cameraBuffer.buffer, 0, sizeof(GPUCameraData)
			};
			vk::DescriptorBufferInfo pointLightBufferInfo = {
				mFrameRenderData[i].pointLightBuffer.buffer, 0, sizeof(GPUPointLightData) * gMaxPointLights
			};
			vk::DescriptorBufferInfo spotLightBufferInfo = {
				mFrameRenderData[i].spotLightBuffer.buffer, 0, sizeof(GPUSpotLightData) * gMaxSpotLights
			};
			vk::DescriptorBufferInfo dirLightBufferInfo = {
				mFrameRenderData[i].directionalLightBuffer.buffer, 0, sizeof(GPUDirLightData) * gMaxDirectionalLights
			};
			vk::DescriptorBufferInfo shadowBufferInfo = {
				mFrameRenderData[i].shadowBuffer.buffer, 0, sizeof(GPUShadowData) * gMaxLights
			};
            vk::DescriptorBufferInfo shadowCascadeBufferInfo = {
                    mFrameRenderData[i].shadowCascadeBuffer.buffer, 0, sizeof(GPUShadowCascadeData) * gMaxLights * gMaxShadowCascadesPerLight
            };
			vk::DescriptorBufferInfo materialBufferInfo = {
				mFrameRenderData[i].materialInstanceBuffer.buffer, 0, sizeof(GPUMaterialInstanceData) * gMaxMaterialInstances
			};

			util::DescriptorBuilder::Begin(mGlobalRenderData.descriptorLayoutCache,
				mGlobalRenderData.descriptorAllocator)
				.BindBuffer(0, &cameraBufferInfo, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex)
				.BindBuffer(1, &pointLightBufferInfo, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eFragment)
				.BindBuffer(2, &spotLightBufferInfo, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eFragment)
				.BindBuffer(3, &dirLightBufferInfo, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eFragment)
				.BindBuffer(4, &shadowBufferInfo, vk::DescriptorType::eStorageBuffer,vk::ShaderStageFlagBits::eFragment)
				.BindBuffer(5, &shadowCascadeBufferInfo, vk::DescriptorType::eStorageBuffer,vk::ShaderStageFlagBits::eFragment)
				.BindBuffer(6, &materialBufferInfo, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eFragment)
				.Build(mFrameRenderData[i].globalDescriptor, mGlobalRenderData.globalSetLayout);

			constexpr uint32_t imageCount = 128;
			uint32_t variableDescCount = imageCount;

			constexpr vk::DescriptorBindingFlags descriptorBindingFlags = { vk::DescriptorBindingFlagBits::ePartiallyBound | vk::DescriptorBindingFlagBits::eVariableDescriptorCount };
			vk::DescriptorSetVariableDescriptorCountAllocateInfo variableDescriptorCountAllocInfo =
			{ 1, &variableDescCount };

			util::DescriptorBuilder::Begin(mGlobalRenderData.descriptorLayoutCache,
				mGlobalRenderData.descriptorAllocator)
				.BindImagesWithoutWrite(0, imageCount, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment, descriptorBindingFlags)
				.AddPNext(&variableDescriptorCountAllocInfo)
				.Build(mFrameRenderData[i].textureDescriptor, mGlobalRenderData.textureSetLayout);

			variableDescCount = gMaxLights;

			util::DescriptorBuilder::Begin(mGlobalRenderData.descriptorLayoutCache,
				mGlobalRenderData.descriptorAllocator)
				.BindImagesWithoutWrite(0, gMaxLights, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment, descriptorBindingFlags)
				.AddPNext(&variableDescriptorCountAllocInfo)
				.Build(mFrameRenderData[i].shadowmapDescriptor, mGlobalRenderData.shadowmapSetLayout);
		}

		mDeletionQueue.PushFunction([=]()
		{
			mGlobalRenderData.descriptorLayoutCache = nullptr;
			mGlobalRenderData.descriptorAllocator = nullptr;
		});
	}

	void RenderSubsystemVK::InitPipelines()
	{
		BuildForwardRendererPipeline();
		BuildShadowPipeline();
	}

	void RenderSubsystemVK::BuildForwardRendererPipeline()
	{
		mForwardVertMod = util::ShaderModule{
			mDevice, fs::path(assets::AssetRegistry::Get()->GetEngineRoot() / "bin" / "vulkan" / "forward_shading" / "forward_shading_vs.spv").string()
		};

		mForwardFragMod = util::ShaderModule{
			mDevice, fs::path(assets::AssetRegistry::Get()->GetEngineRoot() / "bin" / "vulkan" / "forward_shading" / "forward_shading_fs.spv").string()
		};

		constexpr vk::PushConstantRange vertRange = { vk::ShaderStageFlagBits::eVertex, 0, sizeof(GPUVertexShaderPushConstant) };
		constexpr vk::PushConstantRange fragRange = { vk::ShaderStageFlagBits::eFragment, sizeof(GPUVertexShaderPushConstant), sizeof(GPUFragShaderPushConstant) };

		util::PipelineLayoutBuilder plb{};
		mForwardPipelineLayout = plb
		.DescriptorSetLayout(mGlobalRenderData.objectSetLayout)
		.DescriptorSetLayout(mGlobalRenderData.globalSetLayout)
		.DescriptorSetLayout(mGlobalRenderData.textureSetLayout)
		.DescriptorSetLayout(mGlobalRenderData.shadowmapSetLayout)
		.PushConstantRange(vertRange)
		.PushConstantRange(fragRange)
		.CreateUnique(mDevice);

		vk::PipelineDepthStencilStateCreateInfo depthStencilInfo = {
			{}, true, true,
			vk::CompareOp::eLessOrEqual, false, false, {}, {}, 0.0f, 1.0f
		};

		vk::PipelineRenderingCreateInfoKHR pipelineRenderInfo = {
			0, mOffscreenData.imageFormat, mOffscreenData.allocDepthImage.format
		};

		util::PipelineBuilder pb{mSwapchainExtent.width, mSwapchainExtent.height};
		mForwardPipeline = pb
       // Define dynamic state which can change each frame (currently viewport and scissor size)
       .DynamicState(vk::DynamicState::eViewport)
       .DynamicState(vk::DynamicState::eScissor)
       // Define vertex/fragment shaders
       .Shader(vk::ShaderStageFlagBits::eVertex, mForwardVertMod)
       .Shader(vk::ShaderStageFlagBits::eFragment, mForwardFragMod)
       .DepthStencilState(depthStencilInfo)
       // Add rendering info struct
       .AddPNext(&pipelineRenderInfo)
       // Create pipeline
       .CreateUnique(mDevice, m_pipeline_cache, *mForwardPipelineLayout, nullptr);

		mDevice.destroyShaderModule(mForwardVertMod.Module());
		mDevice.destroyShaderModule(mForwardFragMod.Module());

		mDeletionQueue.PushFunction([=]()
		{
			mDevice.destroyPipeline(mForwardPipeline.get());
			mDevice.destroyPipelineLayout(mForwardPipelineLayout.get());

			mForwardPipeline = {};
			mForwardPipelineLayout = {};
		});
	}

	void RenderSubsystemVK::BuildShadowPipeline()
	{
		mShadowVertMod = util::ShaderModule{
			mDevice, fs::path(assets::AssetRegistry::Get()->GetEngineRoot() / "bin" / "vulkan" / "shadowmaps" / "shadowmap_vs.spv").string()
		};

		mShadowFragMod = util::ShaderModule{
			mDevice, fs::path(assets::AssetRegistry::Get()->GetEngineRoot() / "bin" / "vulkan" / "shadowmaps" / "shadowmap_fs.spv").string()
		};

		constexpr vk::PushConstantRange range = { vk::ShaderStageFlagBits::eVertex, 0, sizeof(GPUShadowPushConstant) };

		util::PipelineLayoutBuilder plb{};
		mShadowPipelineLayout = plb
			.DescriptorSetLayout(mGlobalRenderData.objectSetLayout)
			.PushConstantRange(range)
			.CreateUnique(mDevice);

		constexpr vk::PipelineDepthStencilStateCreateInfo depthStencilInfo = {
			{}, true, true,
			vk::CompareOp::eLessOrEqual, false, false, {}, {}, 0.0f, 1.0f
		};

		vk::PipelineRenderingCreateInfoKHR pipelineRenderInfo = {
			0, {}, vk::Format::eD32Sfloat
		};


		util::PipelineBuilder pb{ 1024, 1024 };
		mShadowPipeline = pb
			// Define dynamic state which can change each frame (currently viewport and scissor size)
			.DynamicState(vk::DynamicState::eViewport)
			.DynamicState(vk::DynamicState::eScissor)
			// Define vertex/fragment shaders
			.Shader(vk::ShaderStageFlagBits::eVertex, mShadowVertMod)
			.Shader(vk::ShaderStageFlagBits::eFragment, mShadowFragMod)
			.DepthStencilState(depthStencilInfo)
			// Add rendering info struct
			.AddPNext(&pipelineRenderInfo)
			// Create pipeline
			.CreateUnique(mDevice, m_pipeline_cache, *mShadowPipelineLayout, nullptr);

		mDevice.destroyShaderModule(mShadowFragMod.Module());
		mDevice.destroyShaderModule(mShadowVertMod.Module());

		mDeletionQueue.PushFunction([=]()
		{
			mDevice.destroyPipeline(mShadowPipeline.get());
			mDevice.destroyPipelineLayout(mShadowPipelineLayout.get());

			mShadowPipeline = {};
			mShadowPipelineLayout = {};
		});
	}

	void RenderSubsystemVK::InitImgui()
	{
		// Create Descriptor Pool for ImGui
		vk::DescriptorPoolSize poolSizes[] =
		{
			{vk::DescriptorType::eSampler, 1000},
			{vk::DescriptorType::eCombinedImageSampler, 1000},
			{vk::DescriptorType::eSampledImage, 1000},
			{vk::DescriptorType::eStorageImage, 1000},
			{vk::DescriptorType::eUniformTexelBuffer, 1000},
			{vk::DescriptorType::eStorageTexelBuffer, 1000},
			{vk::DescriptorType::eUniformBuffer, 1000},
			{vk::DescriptorType::eStorageBuffer, 1000},
			{vk::DescriptorType::eUniformBufferDynamic, 1000},
			{vk::DescriptorType::eStorageBufferDynamic, 1000},
			{vk::DescriptorType::eInputAttachment, 1000}
		};

		const vk::DescriptorPoolCreateInfo poolInfo =
		{
			vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, 1000, std::size(poolSizes), poolSizes
		};

		vk::DescriptorPool imguiPool;
		VK_CHECK(mDevice.createDescriptorPool(&poolInfo, nullptr, &imguiPool));

		// Initialize imgui for GLFW
		const auto windowSubsystem = mEngine->GetSubsystem<window::WindowSubsystem>();
		GLFWwindow* glfwWindow = windowSubsystem->GetPrimaryWindow();
		ImGui_ImplGlfw_InitForVulkan(glfwWindow, true);

		std::array<VkFormat,1 > formats = { static_cast<VkFormat>(mSwapchainData.imageFormat) };

		const VkPipelineRenderingCreateInfoKHR pipelineRenderInfo = { VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
			nullptr, 0, 1,
			formats.data(), VK_FORMAT_UNDEFINED, VK_FORMAT_UNDEFINED
		};

		// Initialize imgui for Vulkan
		ImGui_ImplVulkan_InitInfo initInfo = {
			mInstance, mPhysicalDevice, mDevice, mGraphicsQueueFamily,
			mGraphicsQueue, imguiPool, nullptr, static_cast<uint32_t>(mSwapchainData.images.size()),
			static_cast<uint32_t>(mSwapchainData.images.size()), VK_SAMPLE_COUNT_1_BIT, m_pipeline_cache,
			0, true, pipelineRenderInfo
		};

		ImGui_ImplVulkan_Init(&initInfo);

		// Upload ImGui font textures
		util::ImmediateSubmit(this, [=](vk::CommandBuffer cmd)
		{
			ImGui_ImplVulkan_CreateFontsTexture();
		});

		mDeletionQueue.PushFunction([=]()
		{
			mDevice.destroyDescriptorPool(imguiPool, nullptr);
			ImGui_ImplVulkan_Shutdown();
			ImGui_ImplGlfw_Shutdown();
		});
	}

	void RenderSubsystemVK::InitOffscreenImguiTextures(OffscreenData& offscreenData)
	{
		offscreenData.viewportTextures.resize(offscreenData.allocImages.size());

		for (int i = 0; i < offscreenData.allocImages.size(); i++)
		{
			offscreenData.viewportTextures[i] = static_cast<ImTextureID>(ImGui_ImplVulkan_AddTexture(
				mGlobalRenderData.textureSampler,
				offscreenData.allocImages[i].imageView,
				static_cast<VkImageLayout>(vk::ImageLayout::eShaderReadOnlyOptimal)));
		}
	}

	void RenderSubsystemVK::DeinitModules()
	{
		for (auto renderModule : mRenderModuleVector)
		{
			renderModule->Deinitialize();
		}
	}

	void RenderSubsystemVK::UpdateResources()
	{
		for (auto renderModule : mRenderModuleVector)
		{
			renderModule->UpdateResources(mResourceManager.get());
		}

		mResourceManager->CreateAndUpdateResources();
	}

	void RenderSubsystemVK::UpdateGraph()
	{
		mRenderGraph.Reset();

		for (auto renderModule : mRenderModuleVector)
		{
			renderModule->UpdateGraph(mRenderGraph);
		}

		UpdateGraphSemaphores();
		UpdateGraphCommands();
	}

	void RenderSubsystemVK::UpdateGraphSemaphores()
	{
		auto& frameData = GetCurrentFrameData();
		const auto& renderPasses = mRenderGraph.GetRenderPasses();

		std::vector<std::string> semaphoresToRemove;

		// Destroy old semaphores
		for (auto& [name, semaphore] : frameData.semaphores)
		{
			if (renderPasses.find(name) == renderPasses.end())
			{
				mDevice.destroySemaphore(semaphore);
				semaphoresToRemove.push_back(name);
			}
		}

		for (const auto& name : semaphoresToRemove)
		{
			frameData.semaphores.erase(name);
		}

		semaphoresToRemove.clear();

		// Create new semaphores
		vk::SemaphoreCreateInfo semaphoreCreateInfo = { {}, nullptr };
		for (const auto& [name, pass] : renderPasses)
		{
			if (frameData.semaphores.find(name) == frameData.semaphores.end())
			{
				frameData.semaphores.emplace(name, vk::Semaphore{});

				VK_CHECK(mDevice.createSemaphore(&semaphoreCreateInfo, nullptr, &frameData.semaphores.at(name)));
			}
		}
	}

	void RenderSubsystemVK::UpdateGraphCommands()
	{

	}

	void RenderSubsystemVK::PreRender(double deltaTime)
	{
		for (auto renderModule : mRenderModuleVector)
		{
			renderModule->PreRender(deltaTime);
		}
	}

	void RenderSubsystemVK::ProcessComponents()
	{
		const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		const auto registry = enttSubsystem->GetRegistry();

		if (mUpdateRenderables)
		{
			const auto meshView2D = registry->view<const TransformComponent2D, const StaticMeshComponent3D>();
			const auto meshView3D = registry->view<const TransformComponent3D, const StaticMeshComponent3D>();

			mRenderables.clear();

			// Iterate 2D objects
			for (auto [entity, transform, mesh] : meshView2D.each())
			{
				const auto nodeID = enttSubsystem->GetID(entity);

				if (mesh.materialID == gInvalidID || mesh.meshID == gInvalidID)
				{
					continue;
				}

				const auto& matData = mMaterialRegistry->GetMaterialData(mesh.materialID);

				mRenderables.emplace_back(nodeID, mesh.meshID, matData.baseMaterialID, mesh.subMeshIdx);

				if (!mCachedObjectData.Contains(nodeID))
				{
					mCachedObjectData.Emplace(nodeID, GPUObjectData());
				}
			}

			// Iterate 3D objects
			for (auto [entity, transform, mesh] : meshView3D.each())
			{
				const auto node_id = enttSubsystem->GetID(entity);

				if (mesh.materialID == gInvalidID || mesh.meshID == gInvalidID)
				{
					continue;
				}

				const auto& matData = mMaterialRegistry->GetMaterialData(mesh.materialID);

				mRenderables.emplace_back(node_id, mesh.meshID, matData.baseMaterialID, mesh.subMeshIdx);

				if (!mCachedObjectData.Contains(node_id))
				{
					mCachedObjectData.Emplace(node_id, GPUObjectData());
				}
			}

			std::sort(mRenderables.begin(), mRenderables.end());

			for (auto& frameData : mFrameRenderData)
			{
				frameData.copyObjectDataToGPU = true;
			}

			mUpdateRenderables = false;
		}
	}

	void RenderSubsystemVK::UpdateRenderData()
	{
		// Load Meshes
		{
			for (const auto meshID : mMeshesToLoad)
			{
				if (const auto staticMesh = assets::AssetRegistry::Get()->GetAsset<assets::StaticMeshAsset>(meshID))
				{
					mUnifiedGeometryBuffer->AddStaticMesh(staticMesh);
				}
			}

			mMeshesToLoad.clear();

			// Load Materials
			mMaterialRegistry->Update();

			if (mMaterialRegistry->GetMaterialDataNeedsUploaded())
			{
				for (uint32_t i = 0; i < gBufferedFrameCount; i++)
				{
					mFrameRenderData[i].copyMaterialDataToGPU = true;
				}
			}
		}

		// Load Textures
		{
			bool textureDescriptorNeedsUpdated = false;
			for (const auto texID : mTexturesToLoad)
			{
				if (texID != gInvalidID && !mTexData.Contains(texID))
				{
					TextureDataVK texData;
					LoadTexture(texID, texData);

					mTexData.Emplace(texID, texData);

					textureDescriptorNeedsUpdated = true;
				}
			}

			mTexturesToLoad.clear();

			if (textureDescriptorNeedsUpdated == true)
			{
				for (int i = 0; i < gBufferedFrameCount; i++)
				{
					mFrameRenderData[i].textureDescriptorNeedsUpdated = true;
				}
			}
		}
	}

	void RenderSubsystemVK::UpdateShadows()
	{
		if (!mShadowsToUpdate.empty())
		{
			for (int i = 0; i < gBufferedFrameCount; i++)
			{
				mFrameRenderData[i].shadowDescriptorNeedsUpdated = true;
			}
		}

		for (const auto& id : mShadowsToUpdate)
		{
			const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
			auto registry = enttSubsystem->GetRegistry();

			auto entity = enttSubsystem->GetEntity(id);
			auto& shadow = registry->get<ShadowCasterComponent3D>(entity);

			AttachmentDescVK params;
			params.imageSize = ImageSizeVK::Absolute;
			params.type = AttachmentTypeVK::Depth;
			params.format = vk::Format::eD32Sfloat;
			params.width = shadow.width;
			params.height = shadow.height;

			shadow.shadowCascadeIDs.resize(shadow.cascadeCount);
			for (int i = 0; i < shadow.cascadeCount; ++i)
			{
				shadow.shadowCascadeIDs[i] = mResourceManager->CreateOrUpdateAttachment(params);
			}
		}

		mShadowsToUpdate.clear();

		for (const auto& resourceID : mShadowResourcesToDestroy)
		{
			mResourceManager->DestroyResource(resourceID);
		}
	}

	void RenderSubsystemVK::WaitForRenderFence()
	{
		// Wait until GPU has finished rendering last frame. Timeout of 1 second
		VK_CHECK(mDevice.waitForFences(1, &GetCurrentFrameData().renderFence, true, 1000000000));
		VK_CHECK(mDevice.resetFences(1, &GetCurrentFrameData().renderFence));
	}

	void RenderSubsystemVK::UpdateSwapchainAndOffscreen()
	{
		if (mEngine->GetShouldRenderEditorUI())
		{
			const auto editorUISubsystem = mEngine->GetSubsystem<ui::EditorUISubsystem>();
			const ImVec2 viewportSize = editorUISubsystem->GetWindowViewport()->GetViewportSize();

			mRenderExtent.width = static_cast<uint32_t>(viewportSize.x);
			mRenderExtent.height = static_cast<uint32_t>(viewportSize.y);

			if (viewportSize.x != mOffscreenData.extent.width ||
				viewportSize.y != mOffscreenData.extent.height)
			{
				mOffscreenData.resized = true;
				mResourceManager->NotifyRenderExtentResized();
			}
		}
		else
		{
			mRenderExtent = mSwapchainExtent;
		}

		RecreateSwapchain();
		RecreateOffscreen();
	}

	void RenderSubsystemVK::RenderEditorUI()
	{
		if (mEngine->GetShouldRenderEditorUI())
		{
			const auto editorUISubsystem = mEngine->GetSubsystem<ui::EditorUISubsystem>();
			editorUISubsystem->GetWindowViewport()->Draw(mOffscreenData.viewportTextures[mCurrentSwapchainIdx]);

			ImGui::Render();
		}
	}

	void RenderSubsystemVK::RecreateSwapchain()
	{
		// Recreate swapchain when window is resized
		if (mSwapchainData.resized == true)
		{
			for (int i = 0; i < gBufferedFrameCount; i++)
			{
				mFrameRenderData[i].swapchainNeedsUpdated = true;
			}

			mSwapchainDataOld = mSwapchainData;
			mSwapchainDataOld.needsCleaned = true;

			InitSwapchain(mSwapchainData, mSwapchainDataOld.swapchain, mSwapchainExtent);

			mSwapchainData.resized = false;
		}

		if (mSwapchainDataOld.needsCleaned == true)
		{
			if (GetCurrentFrameData().swapchainNeedsUpdated == true)
			{
				GetCurrentFrameData().swapchainNeedsUpdated = false;
			}

			int framesUsingNewSwapchain = 0;
			for (int i = 0; i < gBufferedFrameCount; i++)
			{
				if (mFrameRenderData[i].swapchainNeedsUpdated == false)
				{
					framesUsingNewSwapchain++;
				}
			}

			if (framesUsingNewSwapchain == gBufferedFrameCount)
			{
				CleanSwapchain(mSwapchainDataOld);

				mSwapchainDataOld.needsCleaned = false;
			}
		}
	}

	void RenderSubsystemVK::CleanSwapchain(SwapchainData& swapchainData)
	{
		for (int i = 0; i < swapchainData.imageViews.size(); i++)
		{
			mDevice.destroyImageView(swapchainData.imageViews[i]);
		}

		mDevice.destroySwapchainKHR(swapchainData.swapchain);
	}

	void RenderSubsystemVK::RecreateOffscreen()
	{
		if (mOffscreenData.resized == true)
		{
			for (int i = 0; i < gBufferedFrameCount; i++)
			{
				mFrameRenderData[i].offscreenNeedsUpdated = true;
			}

			mOffscreenDataOld = mOffscreenData;
			mOffscreenDataOld.needsCleaned = true;

			InitOffscreen(mOffscreenData, mRenderExtent, mSwapchainData.images.size());

			if (mEngine->GetShouldRenderEditorUI())
			{
				InitOffscreenImguiTextures(mOffscreenData);
			}

			mOffscreenData.resized = false;
		}

		if (mOffscreenDataOld.needsCleaned == true)
		{
			if (GetCurrentFrameData().offscreenNeedsUpdated == true)
			{
				GetCurrentFrameData().offscreenNeedsUpdated = false;
			}

			int framesUsingNewSwapchain = 0;
			for (int i = 0; i < gBufferedFrameCount; i++)
			{
				if (mFrameRenderData[i].offscreenNeedsUpdated == false)
				{
					framesUsingNewSwapchain++;
				}
			}

			if (framesUsingNewSwapchain == gBufferedFrameCount)
			{
				CleanOffscreen(mOffscreenDataOld);

				mOffscreenDataOld.needsCleaned = false;
			}
		}
	}

	void RenderSubsystemVK::CleanOffscreen(OffscreenData& offscreenData)
	{
		mDevice.destroyImageView(offscreenData.allocDepthImage.imageView);
		mAllocator.destroyImage(offscreenData.allocDepthImage.image, offscreenData.allocDepthImage.allocation);

		for (int i = 0; i < offscreenData.allocImages.size(); i++)
		{
			if (mEngine->GetShouldRenderEditorUI())
			{
				ImGui_ImplVulkan_RemoveTexture(static_cast<VkDescriptorSet>(offscreenData.viewportTextures[i]));
			}

			mDevice.destroyImageView(offscreenData.allocImages[i].imageView);
			mAllocator.destroyImage(offscreenData.allocImages[i].image, offscreenData.allocImages[i].allocation);
		}
	}

	void RenderSubsystemVK::UpdateTextureDescriptors()
	{
		if (mInitialized && GetCurrentFrameData().textureDescriptorNeedsUpdated)
		{
			std::vector<vk::DescriptorImageInfo> textureImageInfos;
			BuildTextureDescriptorInfo(mTexData, textureImageInfos);

			util::DescriptorBuilder::Begin(mGlobalRenderData.descriptorLayoutCache,
			                               mGlobalRenderData.descriptorAllocator)
				.UpdateImages(0, textureImageInfos.size(), textureImageInfos.data(),
				              vk::DescriptorType::eCombinedImageSampler)
				.Update(GetCurrentFrameData().textureDescriptor);

			GetCurrentFrameData().textureDescriptorNeedsUpdated = false;
		}
	}

	void RenderSubsystemVK::UpdateShadowDescriptors()
	{
		if (mInitialized && GetCurrentFrameData().shadowDescriptorNeedsUpdated)
		{
			std::vector<vk::DescriptorImageInfo> shadowImageInfos;
			BuildShadowDescriptorInfo(shadowImageInfos);

			util::DescriptorBuilder::Begin(mGlobalRenderData.descriptorLayoutCache,
				mGlobalRenderData.descriptorAllocator)
				.UpdateImages(0, shadowImageInfos.size(), shadowImageInfos.data(),
					vk::DescriptorType::eCombinedImageSampler)
				.Update(GetCurrentFrameData().shadowmapDescriptor);

			GetCurrentFrameData().shadowDescriptorNeedsUpdated = false;
		}
	}

	void RenderSubsystemVK::PrepareSceneData()
	{
		// Prepare camera data
		const AllocatedBuffer& cameraBuffer = GetCurrentFrameData().cameraBuffer;

		const auto cameraSubystem = mEngine->GetSubsystem<CameraSubsystem>();
		const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		auto registry = enttSubsystem->GetRegistry();

		auto entity = enttSubsystem->GetEntity(cameraSubystem->GetActiveCameraID());
		const auto& camera = registry->get<CameraComponent3D>(entity);

		GPUCameraData camUBO;
		camUBO.proj = camera.proj;
		camUBO.view = camera.view;
		camUBO.viewProj = camera.viewProj;

		memcpy(cameraBuffer.allocInfo.pMappedData, &camUBO, sizeof(GPUCameraData));

		PrepareMaterialData();
		PrepareObjectData();
		PrepareLightData();
		PrepareShadowData();
	}

	void RenderSubsystemVK::PrepareMaterialData()
	{
		if (GetCurrentFrameData().copyMaterialDataToGPU)
		{
			std::vector<GPUMaterialInstanceData> materialData;
			materialData.reserve(mMaterialRegistry->GetAllMaterialData().Size());

			int idx = 0;
			for (auto& matData : mMaterialRegistry->GetAllMaterialData())
			{
				// Update cached material data
				for (int i = 0; i < gNumTexturesPerMat; ++i)
				{
					if (matData.texIDs[i] != 0)
					{
						mMaterialRegistry->GetCachedMaterialData(matData.assetId).texIndices[i] = mTexData[matData.texIDs[i]].idx;
					}
				}

				materialData.push_back(mMaterialRegistry->GetCachedMaterialData(matData.assetId));

				matData.idx = idx;

				idx++;
			}

			util::CopyCPUDataIntoGPUBufferParams params;
			params.dataSize = sizeof(GPUMaterialInstanceData) * materialData.size();
			params.dstBuffer = GetCurrentFrameData().materialInstanceBuffer;
			params.srcData = materialData.data();
			util::CopyCPUDataIntoGPUBuffer(this, params);

			GetCurrentFrameData().copyMaterialDataToGPU = false;
		}
	}

	void RenderSubsystemVK::PrepareObjectData()
	{
		if (!mObjectsToRefresh.empty())
		{
			const auto enkiTSSubsystem = mEngine->GetSubsystem<core::EnkiTSSubsystem>();

			// Calculate t value for rendering interpolated position
			const double t = mEngine->GetAccumulatedTime() / mEngine->GetTimeStepFixed();

			std::vector<UUID> objectsToRefresh;
			objectsToRefresh.reserve(mObjectsToRefresh.size());

			for (const auto id : mObjectsToRefresh)
			{
				objectsToRefresh.push_back(id);
			}

			const auto numObjectsToRefresh = objectsToRefresh.size();

			const uint32_t numThreads = enkiTSSubsystem->GetTaskScheduler()->GetNumTaskThreads();

			// Temp object vectors for writing to by threads
			std::vector<std::vector<std::pair<UUID, GPUObjectData>>> threadObjects;

			threadObjects.resize(numThreads);
			for (auto& threadObject : threadObjects)
			{
				threadObject.reserve(std::ceil(gMaxObjects / numThreads));
			}

			const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
			const auto sceneGraph = mEngine->GetSubsystem<scene::SceneGraphSubsystem>();
			const auto registry = enttSubsystem->GetRegistry();

			enki::TaskSet task(numObjectsToRefresh, [&](enki::TaskSetPartition range, uint32_t threadnum)
			{
				for (uint32_t objectIdx = range.start; objectIdx < range.end; objectIdx++)
				{
					const auto entityID = objectsToRefresh[objectIdx];
					const auto entity = enttSubsystem->GetEntity(entityID);
					auto* node = sceneGraph->GetNode(entityID);

                    TransformComponent3D transform;

					if (auto* transformNode3D = dynamic_cast<TransformNode3D*>(node); transformNode3D)
					{
						transform = transformNode3D->GetGlobalTransform();
					}
					else
					{
						transform = registry->get<TransformComponent3D>(entity);
					}
					
					const auto& mesh = registry->get<StaticMeshComponent3D>(entity);

#ifdef PFN_DOUBLE_PRECISION
					Vector3d position = { 0.0 };
#else
					Vector3f position = { 0.0f };
#endif

					if (registry->any_of<physics::VelocityComponent3D>(entity))
					{
                        const auto& velocity = registry->get<physics::VelocityComponent3D>(entity);;

#ifdef PFN_DOUBLE_PRECISION
						Vector3d interpolatedPosition = tempTransform.position + velocity.linear * mEngine->timeStepFixed();
#else
						Vector3f interpolatedPosition = transform.position + velocity.linear * mEngine->GetTimeStepFixed();
#endif

						position = maths::Lerp(transform.position, interpolatedPosition, t);
					}
					else
					{
						position = transform.position;
					}

					GPUObjectData object = {};

					BuildModelTransform(position, transform.orientationQuat, transform.scale, object.model);
					object.matIdx = mMaterialRegistry->GetMaterialData(mesh.materialID).idx;

					threadObjects[threadnum].emplace_back(entityID, object);
				}
			});

			task.m_MinRange = 500; // Try and ensure each thread gets a minimum of transforms matrices to calculate

			enkiTSSubsystem->GetTaskScheduler()->AddTaskSetToPipe(&task);

			enkiTSSubsystem->GetTaskScheduler()->WaitforTask(&task);

			for (const auto& tempThreadObjects : threadObjects)
			{
				for (const auto& [idx, object] : tempThreadObjects)
				{
					if (mCachedObjectData.Contains(idx))
					{
						mCachedObjectData[idx] = object;
					}
				}
			}

			mObjectsToRefresh.clear();
		}

		if (GetCurrentFrameData().copyObjectDataToGPU)
		{
			std::vector<GPUObjectData> objects = {};
			objects.reserve(gMaxObjects);

			for (const auto& renderable : mRenderables)
			{
				objects.emplace_back(mCachedObjectData[renderable.entityID]);
			}

			util::CopyCPUDataIntoGPUBufferParams params;
			params.dataSize = sizeof(GPUObjectData) * objects.size();
			params.dstBuffer = GetCurrentFrameData().objectBuffer;
			params.srcData = objects.data();
			util::CopyCPUDataIntoGPUBuffer(this, params);

			GetCurrentFrameData().copyObjectDataToGPU = false;
		}
	}

	void RenderSubsystemVK::PrepareLightData()
	{
		// Prepare dynamic light data
		const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		const auto registry = enttSubsystem->GetRegistry();

		int p = 0;
		std::vector<GPUPointLightData> pointLights;
		const auto pointLightView = registry->view<const TransformComponent3D, const PointLightComponent3D>();
		for (auto [entity, transform, pointLight] : pointLightView.each())
		{
			if (p >= gMaxPointLights)
				break;

			pointLights.emplace_back();

			pointLights[p].positionShadowIndex.x = transform.position.x;
			pointLights[p].positionShadowIndex.y = transform.position.y;
			pointLights[p].positionShadowIndex.z = transform.position.z;
			pointLights[p].positionShadowIndex.w = -1.0f;

			// PUFFIN_TODO Shadows are currently disabled for point lights, to be enabled later
			/*if (registry->any_of<ShadowCasterComponent3D>(entity))
			{
				const auto& shadow = registry->get<ShadowCasterComponent3D>(entity);

				pointLights[p].color.w = shadow.shadowIdx;
			}*/

			pointLights[p].color.x = pointLight.color.x;
			pointLights[p].color.y = pointLight.color.y;
			pointLights[p].color.z = pointLight.color.z;
			pointLights[p].color.w = 0.0f;

			pointLights[p].ambientSpecularExponent.x = pointLight.ambientIntensity;
			pointLights[p].ambientSpecularExponent.y = pointLight.specularIntensity;
			pointLights[p].ambientSpecularExponent.z = pointLight.specularExponent;
			pointLights[p].ambientSpecularExponent.w = 0.0f;

			pointLights[p].attenuation.x = pointLight.constantAttenuation;
			pointLights[p].attenuation.y = pointLight.linearAttenuation;
			pointLights[p].attenuation.z = pointLight.quadraticAttenuation;
			pointLights[p].attenuation.w = 0.0f;

			++p;
		}

		int s = 0;
		std::vector<GPUSpotLightData> spotLights;
		const auto spotLightView = registry->view<const TransformComponent3D, SpotLightComponent3D>();
		for (auto [entity, transform, spotLight] : spotLightView.each())
		{
			if (s >= gMaxSpotLights)
				break;

			spotLights.emplace_back();

			spotLights[s].positionShadowIndex.x = transform.position.x;
			spotLights[s].positionShadowIndex.y = transform.position.y;
			spotLights[s].positionShadowIndex.z = transform.position.z;
			spotLights[s].positionShadowIndex.w = -1.0f;

			if (registry->any_of<ShadowCasterComponent3D>(entity))
			{
				const auto& shadow = registry->get<ShadowCasterComponent3D>(entity);

				spotLights[s].color.w = shadow.shadowIdx;
			}

			spotLights[s].color.x = spotLight.color.x;
			spotLights[s].color.y = spotLight.color.y;
			spotLights[s].color.z = spotLight.color.z;
			spotLights[s].color.w = 0.0f;

			spotLights[s].ambientSpecularExponent.x = spotLight.ambientIntensity;
			spotLights[s].ambientSpecularExponent.y = spotLight.specularIntensity;
			spotLights[s].ambientSpecularExponent.z = spotLight.specularExponent;
			spotLights[s].ambientSpecularExponent.w = 0.0f;

			glm::vec4 dir = { 0.5f, -0.5f, 0.0f, 1.0f };

			dir = glm::rotateZ(dir, maths::DegToRad(transform.orientationEulerAngles.roll));
			dir = glm::rotateX(dir, maths::DegToRad(transform.orientationEulerAngles.pitch));
			dir = glm::rotateY(dir, maths::DegToRad(transform.orientationEulerAngles.yaw));

			dir = glm::normalize(dir);

			UUID id = enttSubsystem->GetID(entity);
			if (!mCachedLightDirection.Contains(id))
			{
				mCachedLightDirection.Emplace(id, {});
			}

			mCachedLightDirection[id] = { dir.x, dir.y, dir.z };

			spotLights[s].directionInnerCutoffAngle.x = dir.x;
			spotLights[s].directionInnerCutoffAngle.y = dir.y;
			spotLights[s].directionInnerCutoffAngle.z = dir.z;
			spotLights[s].directionInnerCutoffAngle.w = glm::cos(glm::radians(spotLight.innerCutoffAngle));

			spotLights[s].attenuationOuterCutoffAngle.x = spotLight.constantAttenuation;
			spotLights[s].attenuationOuterCutoffAngle.y = spotLight.linearAttenuation;
			spotLights[s].attenuationOuterCutoffAngle.z = spotLight.quadraticAttenuation;
			spotLights[s].attenuationOuterCutoffAngle.w = glm::cos(glm::radians(spotLight.outerCutoffAngle));

			++s;
		}

		int d = 0;
		std::vector<GPUDirLightData> dirLights;
		const auto dirLightView = registry->view<const TransformComponent3D, DirectionalLightComponent3D>();
		for (auto [entity, transform, dirLight] : dirLightView.each())
		{
			if (d >= gMaxDirectionalLights)
				break;

			dirLights.emplace_back();

			dirLights[d].positionShadowIndex.x = transform.position.x;
			dirLights[d].positionShadowIndex.y = transform.position.y;
			dirLights[d].positionShadowIndex.z = transform.position.z;
			dirLights[d].positionShadowIndex.w = -1.0f;

			/*if (registry->any_of<ShadowCasterComponent3D>(entity))
			{
				const auto& shadow = registry->get<ShadowCasterComponent3D>(entity);

				dirLights[d].color.w = shadow.shadowIdx;
			}*/

			dirLights[d].color.x = dirLight.color.x;
			dirLights[d].color.y = dirLight.color.y;
			dirLights[d].color.z = dirLight.color.z;
			dirLights[d].color.w = 0.0f;

			dirLights[d].ambientSpecularExponent.x = dirLight.ambientIntensity;
			dirLights[d].ambientSpecularExponent.y = dirLight.specularIntensity;
			dirLights[d].ambientSpecularExponent.z = dirLight.specularExponent;
			dirLights[d].ambientSpecularExponent.w = 0.0f;

			glm::vec4 dir = { 0.5f, -0.5f, 0.0f, 1.0f };

			dir = glm::rotateZ(dir, maths::DegToRad(transform.orientationEulerAngles.roll));
			dir = glm::rotateX(dir, maths::DegToRad(transform.orientationEulerAngles.pitch));
			dir = glm::rotateY(dir, maths::DegToRad(transform.orientationEulerAngles.yaw));

			dir = glm::normalize(dir);

			UUID id = enttSubsystem->GetID(entity);
			if (!mCachedLightDirection.Contains(id))
			{
				mCachedLightDirection.Emplace(id, {});
			}

			mCachedLightDirection[id] = { dir.x, dir.y, dir.z };

			dirLights[d].direction.x = dir.x;
			dirLights[d].direction.y = dir.y;
			dirLights[d].direction.z = dir.z;
			dirLights[d].direction.w = 0.0f;

			++d;
		}

		util::CopyCPUDataIntoGPUBufferParams params;

		// Copy point light data to buffer
		params.dataSize = pointLights.size() * sizeof(GPUPointLightData);
		params.dstBuffer = GetCurrentFrameData().pointLightBuffer;
		params.srcData = pointLights.data();

		util::CopyCPUDataIntoGPUBuffer(this, params);

		// Copy spot light data to buffer
		params.dataSize = spotLights.size() * sizeof(GPUSpotLightData);
		params.dstBuffer = GetCurrentFrameData().spotLightBuffer;
		params.srcData = spotLights.data();

		util::CopyCPUDataIntoGPUBuffer(this, params);

		// Copy directional light data to buffer
		params.dataSize = dirLights.size() * sizeof(GPUDirLightData);
		params.dstBuffer = GetCurrentFrameData().directionalLightBuffer;
		params.srcData = dirLights.data();

		util::CopyCPUDataIntoGPUBuffer(this, params);

		auto camSystem = mEngine->GetSubsystem<CameraSubsystem>();
		auto entity = enttSubsystem->GetEntity(camSystem->GetActiveCameraID());
		auto& transform = registry->get<TransformComponent3D>(entity);

		// Prepare light static data
		auto& [viewPos, lightCount] = GetCurrentFrameData().pushConstantFrag;
		viewPos.x = transform.position.x;
		viewPos.y = transform.position.y;
		viewPos.z = transform.position.z;
		viewPos.w = 0.0f;

		lightCount.x = pointLights.size();
		lightCount.y = spotLights.size();
		lightCount.z = dirLights.size();
		lightCount.z = 0.0f;
	}

	void RenderSubsystemVK::PrepareShadowData()
	{
		// Prepare dynamic light data
		const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		const auto registry = enttSubsystem->GetRegistry();

		mShadowsToDraw.clear();

		std::vector<GPUShadowData> shadows;
        std::vector<GPUShadowCascadeData> shadowCascades;

        shadows.reserve(gMaxLights);
        shadowCascades.reserve(gMaxLights * gMaxShadowCascadesPerLight);

		int i = 0;
        int c = 0;

		// PUFFIN_TODO - Implement shadowmaps for point lights

		//int p = 0;
		//const auto pointLightShadowView = registry->view<const TransformComponent3D, const PointLightComponent3D, ShadowCasterComponent3D>();
		//for (auto [entity, transform, pointLight, shadow] : pointLightShadowView.each())
		//{
		//	// Break out of loop of maximum number of lights has been reached
		//	if (p >= gMaxPointLights)
		//	{
		//		break;
		//	}

		//	shadows.emplace_back();

		//	++c;

		//	mShadowsToDraw.push_back(enttSubsystem->get_id(entity));

		//	++i;
		//	++p;
		//}

		int s = 0;
		const auto spotLightShadowView = registry->view<const TransformComponent3D, const SpotLightComponent3D, ShadowCasterComponent3D>();
		for (auto [entity, transform, spotLight, shadow] : spotLightShadowView.each())
		{
			if (s >= gMaxSpotLights)
			{
				break;
			}

			shadows.emplace_back();

			constexpr float nearPlane = 1.f;
			constexpr float farPlane = 100.f;
			const float aspect = float(shadow.width) / float(shadow.height);

			UUID id = enttSubsystem->GetID(entity);

			glm::mat4 lightView = glm::lookAt(static_cast<glm::vec3>(transform.position), static_cast<glm::vec3>(transform.position + mCachedLightDirection[id]), glm::vec3(0, 1, 0));

			glm::mat4 lightProjection = glm::perspective(glm::radians(spotLight.outerCutoffAngle * 2), aspect, nearPlane, farPlane);
			lightProjection[1][1] *= -1;

			shadow.lightViewProj = lightProjection * lightView;

			shadows[i].shadowBiasCascadeIndexAndCount.x = shadow.biasMin;
			shadows[i].shadowBiasCascadeIndexAndCount.y = shadow.biasMax;
			shadows[i].shadowBiasCascadeIndexAndCount.z = c;

			int cascadeCount = 0;

			{
				shadowCascades.emplace_back();
				shadowCascades[c].lightSpaceView = shadow.lightViewProj;
				shadowCascades[c].cascadePlaneDistance = 100.0f;

				cascadeCount++;
			}

			shadows[i].shadowBiasCascadeIndexAndCount.w = cascadeCount;


			++c;

			mShadowsToDraw.push_back(enttSubsystem->GetID(entity));

			++i;
			++s;
		}

		int d = 0;
		const auto dirLightShadowView = registry->view<const TransformComponent3D, const DirectionalLightComponent3D, ShadowCasterComponent3D>();
		for (auto [entity, transform, dirLight, shadow] : dirLightShadowView.each())
		{
			if (d >= gMaxDirectionalLights)
			{
				break;
			}

			shadows.emplace_back();

			const auto camSystem = mEngine->GetSubsystem<CameraSubsystem>();
			const auto activeCamEntity = enttSubsystem->GetEntity(camSystem->GetActiveCameraID());
			auto& camera = registry->get<CameraComponent3D>(activeCamEntity);

			// Calculate camera view frustum vertices
			std::vector<glm::vec4> cameraFrustumVertices;
			util::CalculateCameraFrustum(cameraFrustumVertices, camera.view, camera.proj);

			// Transform to light view space
			std::vector<glm::vec4> camLightViewVertices;
			for (const auto& v : cameraFrustumVertices)
			{
				camLightViewVertices.push_back(shadow.lightView * v);
			}

			// Calculate cam frustum aabb in light space
			const auto cam_light_view_aabb = util::CalculateAABBFromVertices(camLightViewVertices);

			// Recalculate shadow bounds if cam aabb is not fully enclosed by shadow bounds aabb
			if (!util::CheckAABBIsEnclosed(cam_light_view_aabb, shadow.boundsAABB))
			{
				// Average vertices to get centre of view frustum
				Vector3f centre;
				for (const auto& v : cameraFrustumVertices)
				{
					centre += glm::vec3(v);
				}
				centre /= cameraFrustumVertices.size();

				UUID id = enttSubsystem->GetID(entity);

				// Calculate light view
				shadow.lightView = glm::lookAt(static_cast<glm::vec3>(centre - mCachedLightDirection[id]), static_cast<glm::vec3>(centre), glm::vec3(0, 1, 0));

				// Calculate light projection - ortho
				camLightViewVertices.clear();
				for (const auto& v : cameraFrustumVertices)
				{
					camLightViewVertices.push_back(shadow.lightView * v);
				}

				shadow.boundsAABB = util::CalculateAABBFromVertices(camLightViewVertices, shadow.boundsMult);

				glm::mat4 lightProjection = glm::ortho(shadow.boundsAABB.min.x, shadow.boundsAABB.max.x,
					shadow.boundsAABB.min.y, shadow.boundsAABB.max.y, shadow.boundsAABB.min.z, shadow.boundsAABB.max.z);
				lightProjection[1][1] *= -1;

				shadow.lightViewProj = lightProjection * shadow.lightView;
			}

			shadows[i].shadowBiasCascadeIndexAndCount.x = shadow.biasMin;
			shadows[i].shadowBiasCascadeIndexAndCount.y = shadow.biasMax;

			shadowCascades.emplace_back();
			shadowCascades[c].lightSpaceView = shadow.lightViewProj;
			shadowCascades[c].cascadePlaneDistance = shadow.boundsAABB.max.x;

			++c;

			mShadowsToDraw.push_back(enttSubsystem->GetID(entity));

			++i;
			++d;
		}

		util::CopyCPUDataIntoGPUBufferParams params;

		params.dataSize = shadows.size() * sizeof(GPUShadowData);
		params.dstBuffer = GetCurrentFrameData().shadowBuffer;
		params.srcData = shadows.data();
		util::CopyCPUDataIntoGPUBuffer(this, params);

		params.dataSize = shadowCascades.size() * sizeof(GPUShadowCascadeData);
		params.dstBuffer = GetCurrentFrameData().shadowCascadeBuffer;
		params.srcData = shadowCascades.data();
        util::CopyCPUDataIntoGPUBuffer(this, params);
	}

	void RenderSubsystemVK::BuildIndirectCommands()
	{
		if (!mRenderables.empty())
		{
			std::vector<vk::DrawIndexedIndirectCommand> indirectCmds = {};
			indirectCmds.resize(gMaxObjects);

			mDrawBatches.clear();
			mDrawBatches.reserve(mMaterialRegistry->GetAllMaterialData().Size());

			bool newBatch = false;
			int cmdIdx = 0;
			int cmdCount = 0;
			int instanceIdx = 0;
			int instanceCount = 0;
			UUID currentMeshID = mRenderables[0].meshID;
			uint8_t currentSubMeshIdx = mRenderables[0].subMeshIdx;

			MeshDrawBatch drawBatch;
			drawBatch.matID = mRenderables[0].matID;
			drawBatch.cmdIndex = 0;

			indirectCmds[cmdIdx].vertexOffset = mUnifiedGeometryBuffer->MeshVertexOffset(currentMeshID, currentSubMeshIdx);
			indirectCmds[cmdIdx].firstIndex = mUnifiedGeometryBuffer->MeshIndexOffset(currentMeshID, currentSubMeshIdx);
			indirectCmds[cmdIdx].indexCount = mUnifiedGeometryBuffer->MeshIndexCount(currentMeshID, currentSubMeshIdx);
			indirectCmds[cmdIdx].firstInstance = 0;

			constexpr int maxInstancesPerCommand = gMaxObjects;
			constexpr int maxCommandsPerBatch = gMaxObjects;

			for (const auto& [entityID, meshID, matID, subMeshIdx] : mRenderables)
			{
				// Push current draw batch struct to vector when material changes or max commands per batch is exceeded
				if (drawBatch.matID != matID || cmdCount >= maxCommandsPerBatch)
				{
					drawBatch.cmdCount = cmdCount;
					cmdCount = 0;

					mDrawBatches.push_back(drawBatch);

					drawBatch.matID = matID;
					drawBatch.cmdIndex = cmdIdx;

					newBatch = true;
				}

				// Start a new command when a new mesh is encountered, when a new batch is started or when maxInstancesPerCommand is exceeded
				if (currentMeshID != meshID || currentSubMeshIdx != subMeshIdx || newBatch || instanceCount >= maxInstancesPerCommand)
				{
					currentMeshID = meshID;
					currentSubMeshIdx = subMeshIdx;

					indirectCmds[cmdIdx].instanceCount = instanceCount;
					instanceCount = 0;

					cmdIdx++;
					cmdCount++;

					indirectCmds[cmdIdx].vertexOffset = mUnifiedGeometryBuffer->MeshVertexOffset(currentMeshID, currentSubMeshIdx);
					indirectCmds[cmdIdx].firstIndex = mUnifiedGeometryBuffer->MeshIndexOffset(currentMeshID, currentSubMeshIdx);
					indirectCmds[cmdIdx].indexCount = mUnifiedGeometryBuffer->MeshIndexCount(currentMeshID, currentSubMeshIdx);
					indirectCmds[cmdIdx].firstInstance = instanceIdx;

					newBatch = false;
				}

				instanceIdx++;
				instanceCount++;
			}

			// Fill out last command
			indirectCmds[cmdIdx].instanceCount = instanceCount;

			cmdCount++;

			drawBatch.cmdCount = cmdCount;

			// Push final draw batch struct to vector at end of loop
			mDrawBatches.push_back(drawBatch);

			util::CopyCPUDataIntoGPUBufferParams params;
			params.dataSize = indirectCmds.size() * sizeof(vk::DrawIndexedIndirectCommand);
			params.dstBuffer = GetCurrentFrameData().indirectDrawBuffer;
			params.srcData = indirectCmds.data();
			util::CopyCPUDataIntoGPUBuffer(this, params);
		}
	}

	void RenderSubsystemVK::RecordRenderPassCommands()
	{
		for (const auto& [name, renderPass] : mRenderGraph.GetRenderPasses())
		{
			// PUFFIN_TODO - Reenable this line once command buffers creation logic has been implemented

			//renderPass.ExecuteRecordCommandsCallback();
		}
	}

	vk::CommandBuffer& RenderSubsystemVK::RecordShadowCommandBuffer(uint32_t swapchainIdx)
	{
		auto& cmd = GetCurrentFrameData().shadowCommandBuffer;

		// Reset command buffer for recording new commands
		cmd.reset();

		// Begin command buffer execution
		constexpr vk::CommandBufferBeginInfo cmdBeginInfo = {
			vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
			nullptr, nullptr
		};

		VK_CHECK(cmd.begin(&cmdBeginInfo));

		cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, mShadowPipeline.get());

		cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, mShadowPipelineLayout.get(), 0, 1,
			&GetCurrentFrameData().objectDescriptor, 0, nullptr);

		cmd.bindIndexBuffer(mUnifiedGeometryBuffer->GetIndexBuffer().buffer, 0, vk::IndexType::eUint32);

		auto entt_subsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();

		for (auto id : mShadowsToDraw)
		{
			const auto& entity = entt_subsystem->GetEntity(id);
			const auto& shadow = entt_subsystem->GetRegistry()->get<ShadowCasterComponent3D>(entity);

			GPUShadowPushConstant pushConstant;
			pushConstant.vertexBufferAddress = mUnifiedGeometryBuffer->GetVertexBufferAddress();
			pushConstant.lightSpaceView = shadow.lightViewProj;

			cmd.pushConstants(mShadowPipelineLayout.get(), vk::ShaderStageFlagBits::eVertex, 0, sizeof(GPUShadowPushConstant), &pushConstant);

			for (const auto& resourceID : shadow.shadowCascadeIDs)
			{
				DrawShadowmap(cmd, mResourceManager->GetImage(resourceID), { shadow.width, shadow.height });
			}
		}

		cmd.end();

		return cmd;
	}

	void RenderSubsystemVK::DrawShadowmap(vk::CommandBuffer cmd, const AllocatedImage& depthImage, const vk::Extent2D& shadowExtent)
	{
		// Transition color image to color attachment optimal
		vk::ImageSubresourceRange imageSubresourceRange = { vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1 };

		vk::ImageMemoryBarrier offscreenMemoryBarrierToDepth = {
			vk::AccessFlagBits::eNone, vk::AccessFlagBits::eDepthStencilAttachmentWrite,
			vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthAttachmentOptimal, {}, {},
			depthImage.image, imageSubresourceRange
		};

		cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eLateFragmentTests,
			{}, 0, nullptr, 0, nullptr,
			1, & offscreenMemoryBarrierToDepth);

		vk::ClearValue depthClear;
		depthClear.depthStencil.depth = 1.f;

		vk::RenderingAttachmentInfoKHR depthAttachInfo = {
			depthImage.imageView, vk::ImageLayout::eDepthAttachmentOptimal, vk::ResolveModeFlagBits::eNone, {},
			vk::ImageLayout::eUndefined, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, depthClear
		};

		vk::RenderingInfoKHR renderInfo = {
			{}, vk::Rect2D{{0, 0}, shadowExtent}, 1, {}, 0, {}, &depthAttachInfo
		};

		cmd.beginRendering(&renderInfo);

		SetDrawParameters(cmd, shadowExtent);

		// Make a indirect draw call for each material
		for (const auto& drawBatch : mDrawBatches)
		{
			DrawMeshBatch(cmd, drawBatch);
		}

		cmd.endRendering();

		// Transition layout to Shader Read Optimal
		vk::ImageMemoryBarrier offscreenMemoryBarrierToShader = {
			vk::AccessFlagBits::eDepthStencilAttachmentWrite, vk::AccessFlagBits::eNone,
			vk::ImageLayout::eDepthAttachmentOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, {}, {},
			depthImage.image, imageSubresourceRange
		};

		cmd.pipelineBarrier(vk::PipelineStageFlagBits::eLateFragmentTests, vk::PipelineStageFlagBits::eBottomOfPipe,
			{}, 0, nullptr, 0, nullptr,
			1, &offscreenMemoryBarrierToShader);
	}

	vk::CommandBuffer& RenderSubsystemVK::RecordMainCommandBuffer(const RecordMainCommandBufferParams& params)
	{
		auto& cmd = GetCurrentFrameData().mainCommandBuffer;

		// Reset command buffer for recording new commands
		cmd.reset();

		// Begin command buffer execution
		vk::CommandBufferBeginInfo cmd_begin_info = {
			vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
			nullptr, nullptr
		};

		VK_CHECK(cmd.begin(&cmd_begin_info));

		// Transition color image to color attachment optimal
		vk::ImageSubresourceRange image_subresource_range = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};

		vk::ImageMemoryBarrier offscreen_memory_barrier_to_color = {
			vk::AccessFlagBits::eNone, vk::AccessFlagBits::eColorAttachmentWrite,
			vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal, {}, {},
			params.colorImage.image, image_subresource_range
		};

		cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eColorAttachmentOutput,
		                    {}, 0, nullptr, 0, nullptr,
		                    1, &offscreen_memory_barrier_to_color);

		vk::ClearValue color_clear;
		color_clear.color = {0.0f, 0.7f, 0.9f, 1.0f};

		vk::ClearValue depth_clear;
		depth_clear.depthStencil.depth = 1.f;

		// Begin Rendering
		vk::RenderingAttachmentInfoKHR color_attach_info = {
			params.colorImage.imageView, vk::ImageLayout::eColorAttachmentOptimal, vk::ResolveModeFlagBits::eNone, {},
			vk::ImageLayout::eUndefined, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, color_clear
		};

		vk::RenderingAttachmentInfoKHR depth_attach_info = {
			params.depthImage.imageView, vk::ImageLayout::eDepthStencilAttachmentOptimal, vk::ResolveModeFlagBits::eNone, {},
			vk::ImageLayout::eUndefined, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, depth_clear
		};

		vk::RenderingInfoKHR render_info = {
			{}, vk::Rect2D{{0, 0}, params.renderExtent}, 1, {}, 1, &color_attach_info, &depth_attach_info
		};

		cmd.beginRendering(&render_info);

		DrawObjects(cmd, params.renderExtent);

		// End Rendering
		cmd.endRendering();

		// Transition layout to Shader Read Optimal
		vk::ImageMemoryBarrier offscreen_memory_barrier_to_shader = {
			vk::AccessFlagBits::eColorAttachmentWrite, vk::AccessFlagBits::eNone,
			vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, {}, {},
			params.colorImage.image, image_subresource_range
		};

		cmd.pipelineBarrier(vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eBottomOfPipe,
		                    {}, 0, nullptr, 0, nullptr,
		                    1, &offscreen_memory_barrier_to_shader);

		// Finish command buffer recording
		cmd.end();

		return cmd;
	}

	void RenderSubsystemVK::DrawObjects(vk::CommandBuffer cmd, const vk::Extent2D& renderExtent)
	{
		SetDrawParameters(cmd, renderExtent);

		BindBuffersAndDescriptors(cmd);

		// Make a indirect draw call for each material
		for (const auto& draw_batch : mDrawBatches)
		{
			// Use loaded material if id is valid, otherwise use default material
			if (draw_batch.matID != gInvalidID)
			{
				cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, mMaterialRegistry->GetMaterial(draw_batch.matID).pipeline.get());
			}
			else
			{
				cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, mForwardPipeline.get());
			}

			DrawMeshBatch(cmd, draw_batch);
		}
	}

	void RenderSubsystemVK::SetDrawParameters(vk::CommandBuffer cmd, const vk::Extent2D& renderExtent)
	{
		vk::Viewport viewport = {
			0, 0, static_cast<float>(renderExtent.width), static_cast<float>(renderExtent.height), 0.1f, 1.0f
		};
		cmd.setViewport(0, 1, &viewport);

		vk::Rect2D scissor = { {0, 0}, {renderExtent.width, renderExtent.height} };
		cmd.setScissor(0, 1, &scissor);
	}

	void RenderSubsystemVK::BindBuffersAndDescriptors(vk::CommandBuffer cmd)
	{
		std::vector<vk::DescriptorSet> descriptors;
		descriptors.push_back(GetCurrentFrameData().objectDescriptor);
		descriptors.push_back(GetCurrentFrameData().globalDescriptor);
		descriptors.push_back(GetCurrentFrameData().textureDescriptor);
		descriptors.push_back(GetCurrentFrameData().shadowmapDescriptor);

		cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, mForwardPipelineLayout.get(), 0, descriptors.size(),
			descriptors.data(), 0, nullptr);

		
		GPUVertexShaderPushConstant pushConstantVert;
		pushConstantVert.vertexBufferAddress = mUnifiedGeometryBuffer->GetVertexBufferAddress();

		cmd.pushConstants(mForwardPipelineLayout.get(), vk::ShaderStageFlagBits::eVertex, 0, sizeof(GPUVertexShaderPushConstant), &pushConstantVert);
		cmd.pushConstants(mForwardPipelineLayout.get(), vk::ShaderStageFlagBits::eFragment, sizeof(GPUVertexShaderPushConstant), sizeof(GPUFragShaderPushConstant), &GetCurrentFrameData().pushConstantFrag);
		
		cmd.bindIndexBuffer(mUnifiedGeometryBuffer->GetIndexBuffer().buffer, 0, vk::IndexType::eUint32);
	}

	void RenderSubsystemVK::DrawMeshBatch(vk::CommandBuffer cmd, const MeshDrawBatch& meshDrawBatch)
	{
		vk::DeviceSize indirectOffset = meshDrawBatch.cmdIndex * sizeof(vk::DrawIndexedIndirectCommand);
		uint32_t drawStride = sizeof(vk::DrawIndexedIndirectCommand);

		DrawIndirectCommandParams params;
		params.buffer = GetCurrentFrameData().indirectDrawBuffer.buffer;
		params.offset = indirectOffset;
		params.drawCount = meshDrawBatch.cmdCount;
		params.stride = drawStride;

		DrawIndexedIndirectCommand(cmd, params);
	}

	vk::CommandBuffer& RenderSubsystemVK::RecordCopyCommandBuffer(uint32_t swapchainIdx)
	{
		auto& cmd = GetCurrentFrameData().copyCommandBuffer;

		// Reset command buffer for recording new commands
		cmd.reset();

		// Begin command buffer execution
		vk::CommandBufferBeginInfo cmd_begin_info = {
			vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
			nullptr, nullptr
		};

		VK_CHECK(cmd.begin(&cmd_begin_info));

		// Setup pipeline barriers for transitioning image layouts

		vk::ImageSubresourceRange image_subresource_range = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};

		// Offscreen Transition
		vk::ImageMemoryBarrier offscreen_memory_barrier = {
			vk::AccessFlagBits::eNone, vk::AccessFlagBits::eTransferRead,
			vk::ImageLayout::eShaderReadOnlyOptimal, vk::ImageLayout::eTransferSrcOptimal, {}, {},
			mOffscreenData.allocImages[swapchainIdx].image, image_subresource_range
		};

		cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer,
		                    {}, 0, nullptr, 0, nullptr,
		                    1, &offscreen_memory_barrier);

		// Swapchain Transition
		vk::ImageMemoryBarrier swapchain_memory_barrier = {
			vk::AccessFlagBits::eMemoryRead, vk::AccessFlagBits::eTransferWrite,
			vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, {}, {},
			mSwapchainData.images[swapchainIdx], image_subresource_range
		};

		cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer,
		                    {}, 0, nullptr, 0, nullptr,
		                    1, &swapchain_memory_barrier);

		// Blit (Copy with auto format coversion (RGB to BGR)) offscreen to swapchain image
		vk::Offset3D blit_size =
		{
			static_cast<int32_t>(mOffscreenData.extent.width),
			static_cast<int32_t>(mOffscreenData.extent.height),
			1
		};

		std::array<vk::Offset3D, 2> offsets = {};
		offsets[1] = blit_size;

		vk::ImageBlit image_blit_region =
		{
			{vk::ImageAspectFlagBits::eColor, 0, 0, 1}, offsets,
			{vk::ImageAspectFlagBits::eColor, 0, 0, 1}, offsets
		};

		cmd.blitImage(mOffscreenData.allocImages[swapchainIdx].image, vk::ImageLayout::eTransferSrcOptimal,
		              mSwapchainData.images[swapchainIdx], vk::ImageLayout::eTransferDstOptimal, 1, &image_blit_region,
		              vk::Filter::eNearest);

		// Setup pipeline barriers for transitioning image layouts back to default

		// Offscreen Transition
		offscreen_memory_barrier = {
			vk::AccessFlagBits::eTransferRead, vk::AccessFlagBits::eNone,
			vk::ImageLayout::eTransferSrcOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, {}, {},
			mOffscreenData.allocImages[swapchainIdx].image, image_subresource_range
		};

		cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer,
		                    {}, 0, nullptr, 0, nullptr,
		                    1, &offscreen_memory_barrier);

		// Swapchain Transition
		swapchain_memory_barrier = {
			vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eMemoryRead,
			vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::ePresentSrcKHR, {}, {},
			mSwapchainData.images[swapchainIdx], image_subresource_range
		};

		cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer,
		                    {}, 0, nullptr, 0, nullptr,
		                    1, &swapchain_memory_barrier);

		cmd.end();

		return cmd;
	}

	vk::CommandBuffer& RenderSubsystemVK::RecordImguiCommandBuffer(uint32_t swapchainIdx,
	                                                               const vk::Extent2D& renderExtent)
	{
		auto& cmd = GetCurrentFrameData().imguiCommandBuffer;

		// Reset command buffer for recording new commands
		cmd.reset();

		// Begin command buffer execution
		vk::CommandBufferBeginInfo cmd_begin_info = {
			vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
			nullptr, nullptr
		};

		VK_CHECK(cmd.begin(&cmd_begin_info));

		// Transition color image to color attachment optimal
		vk::ImageSubresourceRange image_subresource_range = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };

		vk::ImageMemoryBarrier offscreen_memory_barrier_to_color = {
			vk::AccessFlagBits::eNone, vk::AccessFlagBits::eColorAttachmentWrite,
			vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal, {}, {},
			mSwapchainData.images[swapchainIdx], image_subresource_range
		};

		cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eColorAttachmentOutput,
			{}, 0, nullptr, 0, nullptr,
			1, &offscreen_memory_barrier_to_color);

		vk::ClearValue clear_value;
		clear_value.color = {1.0f, 1.0f, 1.0f, 1.0f};

		// Begin rendering
		vk::RenderingAttachmentInfoKHR color_attach_info = {
			mSwapchainData.imageViews[swapchainIdx], vk::ImageLayout::eColorAttachmentOptimal, vk::ResolveModeFlagBits::eNone, {},
			vk::ImageLayout::eUndefined, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, clear_value
		};

		vk::RenderingInfoKHR render_info = {
			{}, vk::Rect2D{{0, 0}, renderExtent}, 1, {}, 1, &color_attach_info
		};

		cmd.beginRendering(&render_info);

		// Record Imgui Draw Data and draw functions into command buffer
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

		cmd.endRendering();

		// Transition layout for presenting to swapchain
		vk::ImageMemoryBarrier offscreen_memory_barrier_to_shader = {
			vk::AccessFlagBits::eNone, vk::AccessFlagBits::eNone,
			vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR, {}, {},
			mSwapchainData.images[swapchainIdx], image_subresource_range
		};

		cmd.pipelineBarrier(vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eBottomOfPipe,
			{}, 0, nullptr, 0, nullptr,
			1, &offscreen_memory_barrier_to_shader);

		cmd.end();

		return cmd;
	}

	void RenderSubsystemVK::RecordAndSubmitCommands(uint32_t swapchainIdx)
	{
		std::vector<vk::SubmitInfo> submits;

		// Prepare shadow rendering command submit
		std::vector<vk::CommandBuffer*> shadowCommands = {};
		std::vector<vk::Semaphore*> shadowWaitSemaphores = { &GetCurrentFrameData().presentSemaphore };
		std::vector<vk::Semaphore*> shadowSignalSemaphores = { &GetCurrentFrameData().shadowSemaphore };
		vk::PipelineStageFlags shadowWaitStage = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
		if (mRenderShadows)
		{
			shadowCommands.push_back(&RecordShadowCommandBuffer(swapchainIdx));

			vk::SubmitInfo shadowSubmit =
			{
				static_cast<uint32_t>(shadowWaitSemaphores.size()), *shadowWaitSemaphores.data(),
				& shadowWaitStage, static_cast<uint32_t>(shadowCommands.size()), *shadowCommands.data(),
				static_cast<uint32_t>(shadowSignalSemaphores.size()), *shadowSignalSemaphores.data(), nullptr
			};

			submits.push_back(shadowSubmit);
		}

		// Prepare main render command submit
		std::vector<vk::CommandBuffer*> renderCommands = { };
		std::vector<vk::Semaphore*> renderWaitSemaphores = { };
		std::vector<vk::PipelineStageFlags> renderWaitStages = { };
		std::vector<vk::Semaphore*> renderSignalSemaphores = { &GetCurrentFrameData().renderSemaphore };
		{
			RecordMainCommandBufferParams params;
			params.swapchainIdx = swapchainIdx;
			params.renderExtent = mOffscreenData.extent;
			params.colorImage = mOffscreenData.allocImages[swapchainIdx];
			params.depthImage = mOffscreenData.allocDepthImage;

			renderCommands.push_back(&RecordMainCommandBuffer(params));

			if (mRenderShadows)
			{
				renderWaitSemaphores.push_back(&GetCurrentFrameData().shadowSemaphore);
				renderWaitStages.emplace_back(vk::PipelineStageFlagBits::eBottomOfPipe);
			}
			else
			{
				renderWaitSemaphores.push_back(&GetCurrentFrameData().presentSemaphore);
				renderWaitStages.emplace_back(vk::PipelineStageFlagBits::eColorAttachmentOutput);
			}

			vk::SubmitInfo renderSubmit =
			{
				static_cast<uint32_t>(renderWaitSemaphores.size()), *renderWaitSemaphores.data(),
				renderWaitStages.data(), static_cast<uint32_t>(renderCommands.size()), *renderCommands.data(),
				static_cast<uint32_t>(renderSignalSemaphores.size()), *renderSignalSemaphores.data(), nullptr
			};

			submits.push_back(renderSubmit);
		}

		{
			vk::PipelineStageFlags waitStage = { vk::PipelineStageFlagBits::eColorAttachmentOutput };

			if (mEngine->GetShouldRenderEditorUI())
			{
				auto& imguiCmd = RecordImguiCommandBuffer(swapchainIdx, mSwapchainData.extent);

				vk::SubmitInfo imguiSubmit =
				{
					1, &GetCurrentFrameData().renderSemaphore,
					&waitStage, 1, &imguiCmd,
					1, &GetCurrentFrameData().imguiSemaphore, nullptr
				};

				submits.push_back(imguiSubmit);
			}
			else
			{
				auto& copyCmd = RecordCopyCommandBuffer(swapchainIdx);

				vk::SubmitInfo copySubmit =
				{
					1, &GetCurrentFrameData().renderSemaphore,
					&waitStage, 1, &copyCmd,
					1, &GetCurrentFrameData().copySemaphore, nullptr
				};

				submits.push_back(copySubmit);
			}
		}

		VK_CHECK(mGraphicsQueue.submit(submits.size(), submits.data(), GetCurrentFrameData().renderFence));

		vk::Semaphore waitSemaphore;

		if (mEngine->GetShouldRenderEditorUI())
		{
			waitSemaphore = GetCurrentFrameData().imguiSemaphore;
		}
		else
		{
			waitSemaphore = GetCurrentFrameData().copySemaphore;
		}

		vk::PresentInfoKHR presentInfo =
		{
			1, &waitSemaphore, 1, &mSwapchainData.swapchain, &swapchainIdx
		};

		VK_CHECK(mGraphicsQueue.presentKHR(&presentInfo));
	}

	void RenderSubsystemVK::BuildModelTransform(const Vector3f& position, const maths::Quat& orientation, const Vector3f& scale,
	                                         glm::mat4& model)
	{
		const auto scaleM = glm::scale(glm::mat4(1.0f), static_cast<glm::vec3>(scale));

		const auto orientM = glm::toMat4(static_cast<glm::quat>(orientation));

		const auto translateM = glm::translate(glm::mat4(1.0f), static_cast<glm::vec3>(position));

		model = translateM * orientM * scaleM;
	}

	bool RenderSubsystemVK::LoadTexture(UUID texId, TextureDataVK& texData)
	{
		if (const auto texAsset = assets::AssetRegistry::Get()->GetAsset<assets::TextureAsset>(texId); texAsset && texAsset->Load())
		{
			texData.assetId = texId;

			texData.sampler = mGlobalRenderData.textureSampler;

			util::InitTextureParams params;
			params.pixelData = texAsset->Pixels().data();
			params.width = texAsset->TextureWidth();
			params.height = texAsset->TextureHeight();
			params.dataSize = texAsset->TextureSize();
			params.format = gTexFormatVK.at(texAsset->TextureFormat());
			texData.texture = util::InitTexture(this, params);

			texAsset->Unload();

			return true;
		}

		return false;
	}

	void RenderSubsystemVK::UnloadTexture(TextureDataVK& texData) const
	{
		mDevice.destroyImageView(texData.texture.imageView);
		mAllocator.destroyImage(texData.texture.image, texData.texture.allocation);
	}

	void RenderSubsystemVK::BuildTextureDescriptorInfo(MappedVector<UUID, TextureDataVK>& textureData,
	                                                   std::vector<vk::DescriptorImageInfo>& textureImageInfos) const
	{
		textureImageInfos.clear();
		textureImageInfos.reserve(textureData.Size());

		int idx = 0;
		for (auto& texData : textureData)
		{
			vk::DescriptorImageInfo textureImageInfo = {
				texData.sampler, texData.texture.imageView, vk::ImageLayout::eShaderReadOnlyOptimal
			};
			textureImageInfos.push_back(textureImageInfo);

			texData.idx = idx;
			idx++;
		}
	}

	void RenderSubsystemVK::BuildShadowDescriptorInfo(std::vector<vk::DescriptorImageInfo>& shadowImageInfos)
	{
		auto entt_subsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		const auto registry = entt_subsystem->GetRegistry();

		shadowImageInfos.clear();
		shadowImageInfos.reserve(gMaxLights);

		int idx = 0;

		// PUFFIN_TODO - Implement shadow maps for point lights

		/*int p = 0;
		const auto pointLightShadowView = registry->view<const TransformComponent3D, const PointLightComponent3D, ShadowCasterComponent3D>();
		for (auto [entity, transform, pointLight, shadow] : pointLightShadowView.each())
		{
			if (p >= gMaxPointLights)
			{
				break;
			}

			for (const auto& resourceID : shadow.shadowCascadeIDs)
			{
				const auto& allocImage = mResourceManager->GetImage(resourceID);

				vk::DescriptorImageInfo shadowImageInfo = { mGlobalRenderData.shadowmapSampler, allocImage.imageView,
					vk::ImageLayout::eShaderReadOnlyOptimal };

				shadowImageInfos.push_back(shadowImageInfo);
			}

			shadow.shadowIdx = idx;

			++idx;
			++p;
		}*/

		int s = 0;
		const auto spotLightShadowView = registry->view<const TransformComponent3D, const SpotLightComponent3D, ShadowCasterComponent3D>();
		for (auto [entity, transform, spotLight, shadow] : spotLightShadowView.each())
		{
			if (s >= gMaxSpotLights)
			{
				break;
			}

			for (const auto& resourceID : shadow.shadowCascadeIDs)
			{
				const auto& allocImage = mResourceManager->GetImage(resourceID);

				vk::DescriptorImageInfo shadowImageInfo = { mGlobalRenderData.shadowmapSampler, allocImage.imageView,
					vk::ImageLayout::eShaderReadOnlyOptimal };

				shadowImageInfos.push_back(shadowImageInfo);
			}

			shadow.shadowIdx = idx;

			++idx;
			++s;
		}

		int d = 0;
		const auto dirLightShadowView = registry->view<const TransformComponent3D, const DirectionalLightComponent3D, ShadowCasterComponent3D>();
		for (auto [entity, transform, dirLight, shadow] : dirLightShadowView.each())
		{
			if (d >= gMaxDirectionalLights)
			{
				break;
			}

			for (const auto& resourceID : shadow.shadowCascadeIDs)
			{
				const auto& allocImage = mResourceManager->GetImage(resourceID);

				vk::DescriptorImageInfo shadowImageInfo = { mGlobalRenderData.shadowmapSampler, allocImage.imageView,
					vk::ImageLayout::eShaderReadOnlyOptimal };

				shadowImageInfos.push_back(shadowImageInfo);
			}

			shadow.shadowIdx = idx;

			++idx;
			++d;
		}
	}

	FrameRenderData& RenderSubsystemVK::GetCurrentFrameData()
	{
		return mFrameRenderData[GetCurrentFrameIdx()];
	}

	void RenderSubsystemVK::FrameBufferResizeCallback(GLFWwindow* window, const int width, const int height)
	{
		const auto system = static_cast<RenderSubsystemVK*>(glfwGetWindowUserPointer(window));

		system->mSwapchainData.resized = true;
		system->mOffscreenData.resized = true;
		system->mResourceManager->NotifySwapchainResized();
		system->mResourceManager->NotifyRenderExtentResized();
		system->mSwapchainExtent.width = width;
		system->mSwapchainExtent.height = height;
	}
}
