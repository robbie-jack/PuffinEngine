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
	RenderSubystemVK::RenderSubystemVK(const std::shared_ptr<core::Engine>& engine) : Subsystem(engine)
	{
		mName = "RenderSubystemVK";
	}

	RenderSubystemVK::~RenderSubystemVK()
	{
	}

	void RenderSubystemVK::Initialize(core::SubsystemManager* subsystemManager)
	{
		const auto windowSubsystem = subsystemManager->CreateAndInitializeSubsystem<window::WindowSubsystem>();
		const auto enttSubsystem = subsystemManager->CreateAndInitializeSubsystem<ecs::EnTTSubsystem>();

		// Bind callbacks
		const auto registry = enttSubsystem->GetRegistry();

		registry->on_construct<StaticMeshComponent3D>().connect<&RenderSubystemVK::OnUpdateMesh>(this);
		registry->on_update<StaticMeshComponent3D>().connect<&RenderSubystemVK::OnUpdateMesh>(this);
		registry->on_destroy<StaticMeshComponent3D>().connect<&RenderSubystemVK::OnDestroyMeshOrTransform>(this);

		registry->on_construct<TransformComponent2D>().connect<&RenderSubystemVK::OnUpdateTransform>(this);
		registry->on_update<TransformComponent2D>().connect<&RenderSubystemVK::OnUpdateTransform>(this);
		registry->on_destroy<TransformComponent2D>().connect<&RenderSubystemVK::OnDestroyMeshOrTransform>(this);

		registry->on_construct<TransformComponent3D>().connect<&RenderSubystemVK::OnUpdateTransform>(this);
		registry->on_update<TransformComponent3D>().connect<&RenderSubystemVK::OnUpdateTransform>(this);
		registry->on_destroy<TransformComponent3D>().connect<&RenderSubystemVK::OnDestroyMeshOrTransform>(this);

		registry->on_construct<ShadowCasterComponent3D>().connect<&RenderSubystemVK::OnConstructShadowCaster>(this);
		registry->on_update<ShadowCasterComponent3D>().connect<&RenderSubystemVK::OnUpdateShadowCaster>(this);
		registry->on_destroy<ShadowCasterComponent3D>().connect<&RenderSubystemVK::OnDestroyShadowCaster>(this);

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

		InitSwapchain(mSwapchainData, mSwapchainDataOld.swapchain, mWindowSize);

		if (mEngine->GetShouldRenderEditorUI())
		{
			const auto editorUISubsystem = subsystemManager->CreateAndInitializeSubsystem<ui::EditorUISubsystem>();
			const ImVec2 viewportSize = editorUISubsystem->GetWindowViewport()->viewportSize();
			mRenderExtent.width = viewportSize.x;
			mRenderExtent.height = viewportSize.y;
		}
		else
		{
			mRenderExtent = mWindowSize;
		}

		InitOffscreen(mOffscreenData, mRenderExtent, mSwapchainData.images.size());

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
		mCachedObjectData.reserve(gMaxObjects);

		mResourceManager = std::make_unique<ResourceManagerVK>(this);
		mMaterialRegistry = std::make_unique<MaterialRegistryVK>(this);

		mInitialized = true;
		mUpdateRenderables = true;

		const auto settingsManager = mEngine->GetSubsystem<core::SettingsManager>();
		mRenderShadows = settingsManager->Get<bool>("rendering_draw_shadows");
	}

	void RenderSubystemVK::Deinitialize()
	{
		mDevice.waitIdle();

		if (mInitialized)
		{
			for (auto texData : mTexData)
			{
				UnloadTexture(texData);
			}

			mTexData.clear();

			CleanSwapchain(mSwapchainData);

			if (mSwapchainDataOld.needs_cleaned)
			{
				CleanSwapchain(mSwapchainDataOld);
			}

			CleanOffscreen(mOffscreenData);

			if (mOffscreenDataOld.needs_cleaned)
			{
				CleanOffscreen(mOffscreenDataOld);
			}

			mResourceManager = nullptr;
			//m_material_registry = nullptr;

			mDeletionQueue.flush();

			mInitialized = false;
		}
	}

	core::SubsystemType RenderSubystemVK::GetType() const
	{
		return core::SubsystemType::Render;
	}

	double RenderSubystemVK::WaitForLastPresentationAndSampleTime()
	{
		// Wait until GPU has finished presenting last frame. Timeout of 1 second
		VK_CHECK(mDevice.waitForFences(1, &GetCurrentFrameData().presentFence, true, 1000000000));
		VK_CHECK(mDevice.resetFences(1, &GetCurrentFrameData().presentFence));

		VK_CHECK(mDevice.acquireNextImageKHR(mSwapchainData.swapchain, 1000000000, GetCurrentFrameData().presentSemaphore,
				GetCurrentFrameData().presentFence, &mCurrentSwapchainIdx));

		return glfwGetTime();
	}

	void RenderSubystemVK::Render(double deltaTime)
	{
		UpdateRenderData();

		ProcessComponents();

		Draw();
	}

	void RenderSubystemVK::OnUpdateMesh(entt::registry& registry, entt::entity entity)
	{
		const auto mesh = registry.get<StaticMeshComponent3D>(entity);

		if (mesh.meshID == gInvalidID || mesh.materialID == gInvalidID)
		{
			return;
		}

		mMeshesToLoad.insert(mesh.meshID);
		mMaterialRegistry->register_material_instance(mesh.materialID);

		AddRenderable(registry, entity);
	}

	void RenderSubystemVK::OnUpdateTransform(entt::registry& registry, entt::entity entity)
	{
		AddRenderable(registry, entity);
	}

	void RenderSubystemVK::OnDestroyMeshOrTransform(entt::registry& registry, entt::entity entity)
	{
		mUpdateRenderables = true;
	}

	void RenderSubystemVK::AddRenderable(entt::registry& registry, entt::entity entity)
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

	void RenderSubystemVK::OnConstructShadowCaster(entt::registry& registry, entt::entity entity)
	{
		const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		const auto id = enttSubsystem->GetID(entity);
		const auto& shadow = registry.get<ShadowCasterComponent3D>(entity);

		ImageDesc imageDesc;
		imageDesc.image_type = ImageType::Depth;
		imageDesc.format = vk::Format::eD32Sfloat;
		imageDesc.width = shadow.width;
		imageDesc.height = shadow.height;
		imageDesc.depth = 1;

		mShadowConstructEvents.push({ entity, imageDesc });
	}

	void RenderSubystemVK::OnUpdateShadowCaster(entt::registry& registry, entt::entity entity)
	{
		const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		const auto id = enttSubsystem->GetID(entity);
		const auto& shadow = registry.get<ShadowCasterComponent3D>(entity);

		ImageDesc imageDesc;
		imageDesc.image_type = ImageType::Depth;
		imageDesc.format = vk::Format::eD32Sfloat;
		imageDesc.width = shadow.width;
		imageDesc.height = shadow.height;
		imageDesc.depth = 1;

		mShadowUpdateEvents.push({ entity, imageDesc });
	}

	void RenderSubystemVK::OnDestroyShadowCaster(entt::registry& registry, entt::entity entity)
	{
		const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		const auto id = enttSubsystem->GetID(entity);
		auto& shadow = registry.get<ShadowCasterComponent3D>(entity);

		mShadowDestroyEvents.push({ shadow.resourceID });

		shadow.resourceID = gInvalidID;
	}

	void RenderSubystemVK::RegisterTexture(UUID textureID)
	{
		mTexturesToLoad.insert(textureID);
	}

	void RenderSubystemVK::InitVulkan()
	{
		auto windowSubsystem = mEngine->GetSubsystem<window::WindowSubsystem>();
		GLFWwindow* glfwWindow = windowSubsystem->primary_window();

		glfwSetWindowUserPointer(glfwWindow, this);
		glfwSetFramebufferSizeCallback(glfwWindow, FrameBufferResizeCallback);

		// Create Vulkan Instance
		int width, height;
		glfwGetWindowSize(glfwWindow, &width, &height);
		mWindowSize.width = static_cast<unsigned>(width);
		mWindowSize.height = static_cast<unsigned>(height);

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

		mDeletionQueue.pushFunction([=]()
		{
			mAllocator.destroy();

			mDevice.destroy();
			mInstance.destroySurfaceKHR(mSurface);
			vkb::destroy_debug_utils_messenger(mInstance, mDebugMessenger);
			mInstance.destroy();
		});
	}

	void RenderSubystemVK::InitSwapchain(SwapchainData& swapchainData, vk::SwapchainKHR& oldSwapchain,
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
		swapchainData.image_format = static_cast<vk::Format>(vkbSwapchain.image_format);

		// Grab Images
		std::vector<VkImage> images = vkbSwapchain.get_images().value();
		std::vector<VkImageView> imageViews = vkbSwapchain.get_image_views().value();

		swapchainData.images.resize(images.size());
		swapchainData.image_views.resize(imageViews.size());

		for (int i = 0; i < images.size(); i++)
		{
			swapchainData.images[i] = static_cast<vk::Image>(images[i]);
			swapchainData.image_views[i] = static_cast<vk::ImageView>(imageViews[i]);
		}

		images.clear();
		imageViews.clear();
	}

	void RenderSubystemVK::InitOffscreen(OffscreenData& offscreenData, const vk::Extent2D& offscreenExtent,
	                                   const int& offscreenImageCount)
	{
		offscreenData.extent = offscreenExtent;

		const vk::Extent3D imageExtent = {offscreenData.extent.width, offscreenData.extent.height, 1};

		offscreenData.image_format = vk::Format::eR8G8B8A8Unorm;

		util::CreateImageParams createImageParams;

		createImageParams.imageInfo = {
			{}, vk::ImageType::e2D, offscreenData.image_format, imageExtent,
			1, 1, vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
			{
				vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled |
				vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst
			}
		};

		const vk::ImageSubresourceRange subresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};

		createImageParams.imageViewInfo = {
			{}, {}, vk::ImageViewType::e2D, offscreenData.image_format, {}, subresourceRange
		};

		offscreenData.alloc_images.resize(offscreenImageCount);
		for (int i = 0; i < offscreenImageCount; i++)
		{
			offscreenData.alloc_images[i] = util::CreateImage(this, createImageParams);
		}

		util::CreateFormattedImageParams params;
		params.extent = imageExtent;
		params.format = vk::Format::eD32Sfloat;

		offscreenData.alloc_depth_image = util::CreateDepthImage(this, params);
	}

	void RenderSubystemVK::InitCommands()
	{
		vk::CommandPoolCreateInfo commandPoolInfo = {
			vk::CommandPoolCreateFlagBits::eResetCommandBuffer, mGraphicsQueueFamily
		};
		vk::CommandBufferAllocateInfo commandBufferInfo = {{}, vk::CommandBufferLevel::ePrimary, 1};

		// Init Main Command Pools/Buffers
		for (int i = 0; i < gBufferedFrames; i++)
		{
			VK_CHECK(mDevice.createCommandPool(&commandPoolInfo, nullptr, &mFrameRenderData[i].commandPool));

			commandBufferInfo.commandPool = mFrameRenderData[i].commandPool;
			VK_CHECK(mDevice.allocateCommandBuffers(&commandBufferInfo, &mFrameRenderData[i].shadowCommandBuffer));
			VK_CHECK(mDevice.allocateCommandBuffers(&commandBufferInfo, &mFrameRenderData[i].mainCommandBuffer));
			VK_CHECK(mDevice.allocateCommandBuffers(&commandBufferInfo, &mFrameRenderData[i].copyCommandBuffer));
			VK_CHECK(mDevice.allocateCommandBuffers(&commandBufferInfo, &mFrameRenderData[i].imguiCommandBuffer));

			mDeletionQueue.pushFunction([=]()
			{
				mDevice.destroyCommandPool(mFrameRenderData[i].commandPool);
			});
		}

		// Init Upload Context Command Pool/Buffer
		commandPoolInfo = {vk::CommandPoolCreateFlagBits::eResetCommandBuffer, mGraphicsQueueFamily};
		VK_CHECK(mDevice.createCommandPool(&commandPoolInfo, nullptr, &mUploadContext.commandPool));

		commandBufferInfo = {mUploadContext.commandPool, vk::CommandBufferLevel::ePrimary, 1};
		VK_CHECK(mDevice.allocateCommandBuffers(&commandBufferInfo, &mUploadContext.commandBuffer));

		mDeletionQueue.pushFunction([=]()
		{
			mDevice.destroyCommandPool(mUploadContext.commandPool);
		});
	}

	void RenderSubystemVK::InitSyncStructures()
	{
		vk::FenceCreateInfo fenceCreateInfo = {vk::FenceCreateFlagBits::eSignaled, nullptr};
		vk::SemaphoreCreateInfo semaphoreCreateInfo = {{}, nullptr};

		for (int i = 0; i < gBufferedFrames; i++)
		{
			VK_CHECK(mDevice.createFence(&fenceCreateInfo, nullptr, &mFrameRenderData[i].renderFence));
			VK_CHECK(mDevice.createFence(&fenceCreateInfo, nullptr, &mFrameRenderData[i].presentFence));

			VK_CHECK(mDevice.createSemaphore(&semaphoreCreateInfo, nullptr, &mFrameRenderData[i].renderSemaphore));
			VK_CHECK(mDevice.createSemaphore(&semaphoreCreateInfo, nullptr, &mFrameRenderData[i].copySemaphore));
			VK_CHECK(mDevice.createSemaphore(&semaphoreCreateInfo, nullptr, &mFrameRenderData[i].imguiSemaphore));
			VK_CHECK(mDevice.createSemaphore(&semaphoreCreateInfo, nullptr, &mFrameRenderData[i].presentSemaphore));
			VK_CHECK(mDevice.createSemaphore(&semaphoreCreateInfo, nullptr, &mFrameRenderData[i].shadowSemaphore));

			mDeletionQueue.pushFunction([=]()
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

		mDeletionQueue.pushFunction([=]()
		{
			mDevice.destroyFence(mUploadContext.uploadFence);
		});
	}

	void RenderSubystemVK::InitBuffers()
	{
		for (int i = 0; i < gBufferedFrames; i++)
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
			params.bufferUsage = vk::BufferUsageFlagBits::eUniformBuffer;
			mFrameRenderData[i].cameraBuffer = util::CreateBuffer(mAllocator, params);

			params.allocSize = sizeof(GPUPointLightData) * gMaxPointLights;
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

			mDeletionQueue.pushFunction([=]()
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

	void RenderSubystemVK::InitSamplers()
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

		mDeletionQueue.pushFunction([=]()
		{
			mDevice.destroySampler(mGlobalRenderData.textureSampler, nullptr);
			mDevice.destroySampler(mGlobalRenderData.shadowmapSampler, nullptr);
		});
	}

	void RenderSubystemVK::BuildDescriptors()
	{
		// Descriptor Allocator/Cache

		mGlobalRenderData.descriptorAllocator = std::make_shared<util::DescriptorAllocator>(mDevice);
		mGlobalRenderData.descriptorLayoutCache = std::make_shared<util::DescriptorLayoutCache>(mDevice);

		for (int i = 0; i < gBufferedFrames; i++)
		{
			// Global Descriptors

			vk::DescriptorBufferInfo objectBufferInfo = {
				mFrameRenderData[i].objectBuffer.buffer, 0, sizeof(GPUObjectData) * gMaxObjects
			};

			util::DescriptorBuilder::begin(mGlobalRenderData.descriptorLayoutCache,
				mGlobalRenderData.descriptorAllocator)
				.bindBuffer(0, &objectBufferInfo, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eVertex)
				.build(mFrameRenderData[i].objectDescriptor, mGlobalRenderData.objectSetLayout);

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

			util::DescriptorBuilder::begin(mGlobalRenderData.descriptorLayoutCache,
				mGlobalRenderData.descriptorAllocator)
				.bindBuffer(0, &cameraBufferInfo, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex)
				.bindBuffer(1, &pointLightBufferInfo, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eFragment)
				.bindBuffer(2, &spotLightBufferInfo, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eFragment)
				.bindBuffer(3, &dirLightBufferInfo, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eFragment)
				.bindBuffer(4, &shadowBufferInfo, vk::DescriptorType::eStorageBuffer,vk::ShaderStageFlagBits::eFragment)
				.bindBuffer(5, &shadowCascadeBufferInfo, vk::DescriptorType::eStorageBuffer,vk::ShaderStageFlagBits::eFragment)
				.bindBuffer(6, &materialBufferInfo, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eFragment)
				.build(mFrameRenderData[i].globalDescriptor, mGlobalRenderData.globalSetLayout);

			constexpr uint32_t imageCount = 128;
			uint32_t variableDescCount = imageCount;

			constexpr vk::DescriptorBindingFlags descriptorBindingFlags = { vk::DescriptorBindingFlagBits::ePartiallyBound | vk::DescriptorBindingFlagBits::eVariableDescriptorCount };
			vk::DescriptorSetVariableDescriptorCountAllocateInfo variableDescriptorCountAllocInfo =
			{ 1, &variableDescCount };

			util::DescriptorBuilder::begin(mGlobalRenderData.descriptorLayoutCache,
				mGlobalRenderData.descriptorAllocator)
				.bindImagesWithoutWrite(0, imageCount, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment, descriptorBindingFlags)
				.addPNext(&variableDescriptorCountAllocInfo)
				.build(mFrameRenderData[i].textureDescriptor, mGlobalRenderData.textureSetLayout);

			variableDescCount = gMaxLights;

			util::DescriptorBuilder::begin(mGlobalRenderData.descriptorLayoutCache,
				mGlobalRenderData.descriptorAllocator)
				.bindImagesWithoutWrite(0, gMaxLights, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment, descriptorBindingFlags)
				.addPNext(&variableDescriptorCountAllocInfo)
				.build(mFrameRenderData[i].shadowmapDescriptor, mGlobalRenderData.shadowmapSetLayout);
		}

		mDeletionQueue.pushFunction([=]()
		{
			mGlobalRenderData.descriptorLayoutCache = nullptr;
			mGlobalRenderData.descriptorAllocator = nullptr;
		});
	}

	void RenderSubystemVK::InitPipelines()
	{
		BuildForwardRendererPipeline();
		BuildShadowPipeline();
	}

	void RenderSubystemVK::BuildForwardRendererPipeline()
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
		.descriptorSetLayout(mGlobalRenderData.objectSetLayout)
		.descriptorSetLayout(mGlobalRenderData.globalSetLayout)
		.descriptorSetLayout(mGlobalRenderData.textureSetLayout)
		.descriptorSetLayout(mGlobalRenderData.shadowmapSetLayout)
		.pushConstantRange(vertRange)
		.pushConstantRange(fragRange)
		.createUnique(mDevice);

		vk::PipelineDepthStencilStateCreateInfo depthStencilInfo = {
			{}, true, true,
			vk::CompareOp::eLessOrEqual, false, false, {}, {}, 0.0f, 1.0f
		};

		vk::PipelineRenderingCreateInfoKHR pipelineRenderInfo = {
			0, mOffscreenData.image_format, mOffscreenData.alloc_depth_image.format
		};

		util::PipelineBuilder pb{mWindowSize.width, mWindowSize.height};
		mForwardPipeline = pb
       // Define dynamic state which can change each frame (currently viewport and scissor size)
       .dynamicState(vk::DynamicState::eViewport)
       .dynamicState(vk::DynamicState::eScissor)
       // Define vertex/fragment shaders
       .shader(vk::ShaderStageFlagBits::eVertex, mForwardVertMod)
       .shader(vk::ShaderStageFlagBits::eFragment, mForwardFragMod)
       .depthStencilState(depthStencilInfo)
       // Add rendering info struct
       .addPNext(&pipelineRenderInfo)
       // Create pipeline
       .createUnique(mDevice, m_pipeline_cache, *mForwardPipelineLayout, nullptr);

		mDevice.destroyShaderModule(mForwardVertMod.module());
		mDevice.destroyShaderModule(mForwardFragMod.module());

		mDeletionQueue.pushFunction([=]()
		{
			mDevice.destroyPipeline(mForwardPipeline.get());
			mDevice.destroyPipelineLayout(mForwardPipelineLayout.get());

			mForwardPipeline = {};
			mForwardPipelineLayout = {};
		});
	}

	void RenderSubystemVK::BuildShadowPipeline()
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
			.descriptorSetLayout(mGlobalRenderData.objectSetLayout)
			.pushConstantRange(range)
			.createUnique(mDevice);

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
			.dynamicState(vk::DynamicState::eViewport)
			.dynamicState(vk::DynamicState::eScissor)
			// Define vertex/fragment shaders
			.shader(vk::ShaderStageFlagBits::eVertex, mShadowVertMod)
			.shader(vk::ShaderStageFlagBits::eFragment, mShadowFragMod)
			.depthStencilState(depthStencilInfo)
			// Add rendering info struct
			.addPNext(&pipelineRenderInfo)
			// Create pipeline
			.createUnique(mDevice, m_pipeline_cache, *mShadowPipelineLayout, nullptr);

		mDevice.destroyShaderModule(mShadowFragMod.module());
		mDevice.destroyShaderModule(mShadowVertMod.module());

		mDeletionQueue.pushFunction([=]()
		{
			mDevice.destroyPipeline(mShadowPipeline.get());
			mDevice.destroyPipelineLayout(mShadowPipelineLayout.get());

			mShadowPipeline = {};
			mShadowPipelineLayout = {};
		});
	}

	void RenderSubystemVK::InitImgui()
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
		GLFWwindow* glfwWindow = windowSubsystem->primary_window();
		ImGui_ImplGlfw_InitForVulkan(glfwWindow, true);

		std::array<VkFormat,1 > formats = { static_cast<VkFormat>(mSwapchainData.image_format) };

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

		mDeletionQueue.pushFunction([=]()
		{
			mDevice.destroyDescriptorPool(imguiPool, nullptr);
			ImGui_ImplVulkan_Shutdown();
			ImGui_ImplGlfw_Shutdown();
		});
	}

	void RenderSubystemVK::InitOffscreenImguiTextures(OffscreenData& offscreenData)
	{
		offscreenData.viewport_textures.resize(offscreenData.alloc_images.size());

		for (int i = 0; i < offscreenData.alloc_images.size(); i++)
		{
			offscreenData.viewport_textures[i] = static_cast<ImTextureID>(ImGui_ImplVulkan_AddTexture(
				mGlobalRenderData.textureSampler,
				offscreenData.alloc_images[i].image_view,
				static_cast<VkImageLayout>(vk::ImageLayout::eShaderReadOnlyOptimal)));
		}
	}

	void RenderSubystemVK::ProcessComponents()
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

				const auto& matData = mMaterialRegistry->get_material_data(mesh.materialID);

				mRenderables.emplace_back(nodeID, mesh.meshID, matData.baseMaterialID, mesh.subMeshIdx);

				if (!mCachedObjectData.contains(nodeID))
				{
					mCachedObjectData.emplace(nodeID, GPUObjectData());
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

				const auto& matData = mMaterialRegistry->get_material_data(mesh.materialID);

				mRenderables.emplace_back(node_id, mesh.meshID, matData.baseMaterialID, mesh.subMeshIdx);

				if (!mCachedObjectData.contains(node_id))
				{
					mCachedObjectData.emplace(node_id, GPUObjectData());
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

	void RenderSubystemVK::UpdateRenderData()
	{
		// Load Meshes
		{
			for (const auto meshID : mMeshesToLoad)
			{
				if (const auto staticMesh = assets::AssetRegistry::Get()->GetAsset<assets::StaticMeshAsset>(meshID))
				{
					mResourceManager->add_static_mesh(staticMesh);
				}
			}

			mMeshesToLoad.clear();

			// Load Materials
			mMaterialRegistry->update();

			if (mMaterialRegistry->material_data_needs_uploaded())
			{
				for (uint32_t i = 0; i < gBufferedFrames; i++)
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
				if (texID != gInvalidID && !mTexData.contains(texID))
				{
					TextureDataVK texData;
					LoadTexture(texID, texData);

					mTexData.emplace(texID, texData);

					textureDescriptorNeedsUpdated = true;
				}
			}

			mTexturesToLoad.clear();

			if (textureDescriptorNeedsUpdated == true)
			{
				for (int i = 0; i < gBufferedFrames; i++)
				{
					mFrameRenderData[i].textureDescriptorNeedsUpdated = true;
				}
			}
		}

		ConstructShadows();
		UpdateShadows();
		DestroyShadows();
	}

	void RenderSubystemVK::ConstructShadows()
	{
		bool shadowDescriptorNeedsUpdated = false;

		while(!mShadowConstructEvents.empty())
		{
			ShadowConstructEvent shadowEvent{};
			mShadowConstructEvents.pop(shadowEvent);

			const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
			auto registry = enttSubsystem->GetRegistry();
			auto& shadow = registry->get<ShadowCasterComponent3D>(shadowEvent.entity);

			shadow.resourceID = mResourceManager->add_images(shadowEvent.imageDesc, mFramesInFlightCount * shadow.cascadeCount);

			shadowDescriptorNeedsUpdated |= true;
		}

		if (shadowDescriptorNeedsUpdated == true)
		{
			for (int i = 0; i < gBufferedFrames; i++)
			{
				mFrameRenderData[i].shadowDescriptorNeedsUpdated = true;
			}
		}
	}

	void RenderSubystemVK::UpdateShadows()
	{
		bool shadowDescriptorNeedsUpdated = false;

		std::vector<ShadowUpdateEvent> shadowUpdateEventsStillInUse;
		while (!mShadowUpdateEvents.empty())
		{
			ShadowUpdateEvent shadowEvent{};
			mShadowUpdateEvents.pop(shadowEvent);

			const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
			auto registry = enttSubsystem->GetRegistry();
			auto& shadow = registry->get<ShadowCasterComponent3D>(shadowEvent.entity);

			if (mResourceManager->image_exists(shadow.resourceID))
			{
				const int firstImageIndex = GetCurrentFrameIdx() * shadow.cascadeCount;
				for (int i = 0; i < shadow.cascadeCount; ++i)
				{
					mResourceManager->update_image(shadow.resourceID, shadowEvent.imageDesc, firstImageIndex + i);
				}

				shadowEvent.frameCount++;

				if (shadowEvent.frameCount < mFramesInFlightCount)
				{
					shadowUpdateEventsStillInUse.push_back(shadowEvent);
				}
			}

			shadowDescriptorNeedsUpdated |= true;
		}

		for (const auto& shadowEvent : shadowUpdateEventsStillInUse)
		{
			mShadowUpdateEvents.push(shadowEvent);
		}

		shadowUpdateEventsStillInUse.clear();

		if (shadowDescriptorNeedsUpdated == true)
		{
			for (int i = 0; i < gBufferedFrames; i++)
			{
				mFrameRenderData[i].shadowDescriptorNeedsUpdated = true;
			}
		}
	}

	void RenderSubystemVK::DestroyShadows()
	{
		std::vector<ShadowDestroyEvent> shadowDestroyEventsStillInUse;
		while (!mShadowDestroyEvents.empty())
		{
			ShadowDestroyEvent shadowEvent{};
			mShadowDestroyEvents.pop(shadowEvent);

			if (mResourceManager->image_exists(shadowEvent.resourceID))
			{
				shadowEvent.frameCount++;

				if (shadowEvent.frameCount < mFramesInFlightCount)
				{
					shadowDestroyEventsStillInUse.push_back(shadowEvent);
				}
				else
				{
					mResourceManager->destroy_images(shadowEvent.resourceID);
				}
			}
		}

		for (const auto& shadowEvent : shadowDestroyEventsStillInUse)
		{
			mShadowDestroyEvents.push(shadowEvent);
		}

		shadowDestroyEventsStillInUse.clear();
	}

	void RenderSubystemVK::Draw()
	{
		// Wait until GPU has finished rendering last frame. Timeout of 1 second
		VK_CHECK(mDevice.waitForFences(1, &GetCurrentFrameData().renderFence, true, 1000000000));
		VK_CHECK(mDevice.resetFences(1, &GetCurrentFrameData().renderFence));

		if (mEngine->GetShouldRenderEditorUI())
		{
			const auto editorUISubsystem = mEngine->GetSubsystem<ui::EditorUISubsystem>();
			const ImVec2 viewportSize = editorUISubsystem->GetWindowViewport()->viewportSize();

			mRenderExtent.width = static_cast<uint32_t>(viewportSize.x);
			mRenderExtent.height = static_cast<uint32_t>(viewportSize.y);

			if (viewportSize.x != mOffscreenData.extent.width ||
				viewportSize.y != mOffscreenData.extent.height)
			{
				mOffscreenData.resized = true;
			}
		}
		else
		{
			mRenderExtent = mWindowSize;
		}

		RecreateSwapchain();
		RecreateOffscreen();

		if (mEngine->GetShouldRenderEditorUI())
		{
			const auto editorUISubsystem = mEngine->GetSubsystem<ui::EditorUISubsystem>();
			editorUISubsystem->GetWindowViewport()->draw(mOffscreenData.viewport_textures[mCurrentSwapchainIdx]);

			ImGui::Render();
		}

		m_draw_calls = 0;

		// Prepare textures, scene data & indirect commands for rendering
		UpdateTextureDescriptors();
		UpdateShadowDescriptors();
		PrepareSceneData();
		BuildIndirectCommands();

		RecordAndSubmitCommands(mCurrentSwapchainIdx);

		m_frame_count++;
	}

	void RenderSubystemVK::RecreateSwapchain()
	{
		// Recreate swapchain when window is resized
		if (mSwapchainData.resized == true)
		{
			for (int i = 0; i < gBufferedFrames; i++)
			{
				mFrameRenderData[i].swapchainNeedsUpdated = true;
			}

			mSwapchainDataOld = mSwapchainData;
			mSwapchainDataOld.needs_cleaned = true;

			InitSwapchain(mSwapchainData, mSwapchainDataOld.swapchain, mWindowSize);

			mSwapchainData.resized = false;
		}

		if (mSwapchainDataOld.needs_cleaned == true)
		{
			if (GetCurrentFrameData().swapchainNeedsUpdated == true)
			{
				GetCurrentFrameData().swapchainNeedsUpdated = false;
			}

			int framesUsingNewSwapchain = 0;
			for (int i = 0; i < gBufferedFrames; i++)
			{
				if (mFrameRenderData[i].swapchainNeedsUpdated == false)
				{
					framesUsingNewSwapchain++;
				}
			}

			if (framesUsingNewSwapchain == gBufferedFrames)
			{
				CleanSwapchain(mSwapchainDataOld);

				mSwapchainDataOld.needs_cleaned = false;
			}
		}
	}

	void RenderSubystemVK::CleanSwapchain(SwapchainData& swapchainData)
	{
		for (int i = 0; i < swapchainData.image_views.size(); i++)
		{
			mDevice.destroyImageView(swapchainData.image_views[i]);
		}

		mDevice.destroySwapchainKHR(swapchainData.swapchain);
	}

	void RenderSubystemVK::RecreateOffscreen()
	{
		if (mOffscreenData.resized == true)
		{
			for (int i = 0; i < gBufferedFrames; i++)
			{
				mFrameRenderData[i].offscreenNeedsUpdated = true;
			}

			mOffscreenDataOld = mOffscreenData;
			mOffscreenDataOld.needs_cleaned = true;

			InitOffscreen(mOffscreenData, mRenderExtent, mSwapchainData.images.size());

			if (mEngine->GetShouldRenderEditorUI())
			{
				InitOffscreenImguiTextures(mOffscreenData);
			}

			mOffscreenData.resized = false;
		}

		if (mOffscreenDataOld.needs_cleaned == true)
		{
			if (GetCurrentFrameData().offscreenNeedsUpdated == true)
			{
				GetCurrentFrameData().offscreenNeedsUpdated = false;
			}

			int framesUsingNewSwapchain = 0;
			for (int i = 0; i < gBufferedFrames; i++)
			{
				if (mFrameRenderData[i].offscreenNeedsUpdated == false)
				{
					framesUsingNewSwapchain++;
				}
			}

			if (framesUsingNewSwapchain == gBufferedFrames)
			{
				CleanOffscreen(mOffscreenDataOld);

				mOffscreenDataOld.needs_cleaned = false;
			}
		}
	}

	void RenderSubystemVK::CleanOffscreen(OffscreenData& offscreenData)
	{
		mDevice.destroyImageView(offscreenData.alloc_depth_image.image_view);
		mAllocator.destroyImage(offscreenData.alloc_depth_image.image, offscreenData.alloc_depth_image.allocation);

		for (int i = 0; i < offscreenData.alloc_images.size(); i++)
		{
			if (mEngine->GetShouldRenderEditorUI())
			{
				ImGui_ImplVulkan_RemoveTexture(static_cast<VkDescriptorSet>(offscreenData.viewport_textures[i]));
			}

			mDevice.destroyImageView(offscreenData.alloc_images[i].image_view);
			mAllocator.destroyImage(offscreenData.alloc_images[i].image, offscreenData.alloc_images[i].allocation);
		}
	}

	void RenderSubystemVK::UpdateTextureDescriptors()
	{
		if (mInitialized && GetCurrentFrameData().textureDescriptorNeedsUpdated)
		{
			std::vector<vk::DescriptorImageInfo> textureImageInfos;
			BuildTextureDescriptorInfo(mTexData, textureImageInfos);

			util::DescriptorBuilder::begin(mGlobalRenderData.descriptorLayoutCache,
			                               mGlobalRenderData.descriptorAllocator)
				.updateImages(0, textureImageInfos.size(), textureImageInfos.data(),
				              vk::DescriptorType::eCombinedImageSampler)
				.update(GetCurrentFrameData().textureDescriptor);

			GetCurrentFrameData().textureDescriptorNeedsUpdated = false;
		}
	}

	void RenderSubystemVK::UpdateShadowDescriptors()
	{
		if (mInitialized && GetCurrentFrameData().shadowDescriptorNeedsUpdated)
		{
			std::vector<vk::DescriptorImageInfo> shadowImageInfos;
			BuildShadowDescriptorInfo(shadowImageInfos);

			util::DescriptorBuilder::begin(mGlobalRenderData.descriptorLayoutCache,
				mGlobalRenderData.descriptorAllocator)
				.updateImages(0, shadowImageInfos.size(), shadowImageInfos.data(),
					vk::DescriptorType::eCombinedImageSampler)
				.update(GetCurrentFrameData().shadowmapDescriptor);

			GetCurrentFrameData().shadowDescriptorNeedsUpdated = false;
		}
	}

	void RenderSubystemVK::PrepareSceneData()
	{
		// Prepare camera data
		const AllocatedBuffer& cameraBuffer = GetCurrentFrameData().cameraBuffer;

		const auto cameraSubystem = mEngine->GetSubsystem<CameraSubystem>();
		const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		auto registry = enttSubsystem->GetRegistry();

		auto entity = enttSubsystem->GetEntity(cameraSubystem->GetActiveCameraID());
		const auto& camera = registry->get<CameraComponent3D>(entity);

		GPUCameraData camUBO;
		camUBO.proj = camera.proj;
		camUBO.view = camera.view;
		camUBO.view_proj = camera.viewProj;

		memcpy(cameraBuffer.alloc_info.pMappedData, &camUBO, sizeof(GPUCameraData));

		PrepareMaterialData();
		PrepareObjectData();
		PrepareLightData();
		PrepareShadowData();
	}

	void RenderSubystemVK::PrepareMaterialData()
	{
		if (GetCurrentFrameData().copyMaterialDataToGPU)
		{
			std::vector<GPUMaterialInstanceData> materialData;
			materialData.reserve(mMaterialRegistry->get_material_data().size());

			int idx = 0;
			for (auto& matData : mMaterialRegistry->get_material_data())
			{
				// Update cached material data
				for (int i = 0; i < gNumTexturesPerMat; ++i)
				{
					if (matData.texIDs[i] != 0)
					{
						mMaterialRegistry->get_cached_material_data(matData.assetId).tex_indices[i] = mTexData[matData.texIDs[i]].idx;
					}
				}

				materialData.push_back(mMaterialRegistry->get_cached_material_data(matData.assetId));

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

	void RenderSubystemVK::PrepareObjectData()
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

                    TransformComponent3D& transform = registry->get<TransformComponent3D>(entity);

					if (auto* transformNode3D = dynamic_cast<TransformNode3D*>(node); transformNode3D)
					{
						transform = transformNode3D->GetGlobalTransform();
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

						position = maths::lerp(transform.position, interpolatedPosition, t);
					}
					else
					{
						position = transform.position;
					}

					GPUObjectData object = {};

					BuildModelTransform(position, transform.orientationQuat, transform.scale, object.model);
					object.mat_idx = mMaterialRegistry->get_material_data(mesh.materialID).idx;

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
					if (mCachedObjectData.contains(idx))
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

	void RenderSubystemVK::PrepareLightData()
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

			dir = glm::rotateZ(dir, maths::deg_to_rad(transform.orientationEulerAngles.roll));
			dir = glm::rotateX(dir, maths::deg_to_rad(transform.orientationEulerAngles.pitch));
			dir = glm::rotateY(dir, maths::deg_to_rad(transform.orientationEulerAngles.yaw));

			dir = glm::normalize(dir);

			spotLight.direction = { dir.x, dir.y, dir.z };

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

			dir = glm::rotateZ(dir, maths::deg_to_rad(transform.orientationEulerAngles.roll));
			dir = glm::rotateX(dir, maths::deg_to_rad(transform.orientationEulerAngles.pitch));
			dir = glm::rotateY(dir, maths::deg_to_rad(transform.orientationEulerAngles.yaw));

			dir = glm::normalize(dir);

			dirLight.direction = { dir.x, dir.y, dir.z };

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

		auto camSystem = mEngine->GetSubsystem<CameraSubystem>();
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

	void RenderSubystemVK::PrepareShadowData()
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

			glm::mat4 lightView = glm::lookAt(static_cast<glm::vec3>(transform.position), static_cast<glm::vec3>(transform.position + spotLight.direction), glm::vec3(0, 1, 0));

			glm::mat4 lightProjection = glm::perspective(glm::radians(spotLight.outerCutoffAngle * 2), aspect, nearPlane, farPlane);
			lightProjection[1][1] *= -1;

			shadow.lightViewProj = lightProjection * lightView;

			shadows[i].shadow_bias.x = shadow.biasMin;
			shadows[i].shadow_bias.y = shadow.biasMax;

			shadowCascades.emplace_back();
			shadowCascades[c].light_space_view = shadow.lightViewProj;
			shadowCascades[c].cascade_plane_distance = 100.0f;

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

			const auto camSystem = mEngine->GetSubsystem<CameraSubystem>();
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

				// Calculate light view
				shadow.lightView = glm::lookAt(static_cast<glm::vec3>(centre - dirLight.direction), static_cast<glm::vec3>(centre), glm::vec3(0, 1, 0));

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

			shadows[i].shadow_bias.x = shadow.biasMin;
			shadows[i].shadow_bias.y = shadow.biasMax;

			shadowCascades.emplace_back();
			shadowCascades[c].light_space_view = shadow.lightViewProj;
			shadowCascades[c].cascade_plane_distance = shadow.boundsAABB.max.x;

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

	void RenderSubystemVK::BuildIndirectCommands()
	{
		if (!mRenderables.empty())
		{
			std::vector<vk::DrawIndexedIndirectCommand> indirectCmds = {};
			indirectCmds.resize(gMaxObjects);

			mDrawBatches.clear();
			mDrawBatches.reserve(mMaterialRegistry->get_material_data().size());

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

			indirectCmds[cmdIdx].vertexOffset = mResourceManager->geometry_buffer()->MeshVertexOffset(currentMeshID, currentSubMeshIdx);
			indirectCmds[cmdIdx].firstIndex = mResourceManager->geometry_buffer()->MeshIndexOffset(currentMeshID, currentSubMeshIdx);
			indirectCmds[cmdIdx].indexCount = mResourceManager->geometry_buffer()->MeshIndexCount(currentMeshID, currentSubMeshIdx);
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

					indirectCmds[cmdIdx].vertexOffset = mResourceManager->geometry_buffer()->MeshVertexOffset(currentMeshID, currentSubMeshIdx);
					indirectCmds[cmdIdx].firstIndex = mResourceManager->geometry_buffer()->MeshIndexOffset(currentMeshID, currentSubMeshIdx);
					indirectCmds[cmdIdx].indexCount = mResourceManager->geometry_buffer()->MeshIndexCount(currentMeshID, currentSubMeshIdx);
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

	vk::CommandBuffer& RenderSubystemVK::RecordShadowCommandBuffer(uint32_t swapchainIdx)
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

		cmd.bindIndexBuffer(mResourceManager->geometry_buffer()->GetIndexBuffer().buffer, 0, vk::IndexType::eUint32);

		auto entt_subsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();

		for (auto id : mShadowsToDraw)
		{
			const auto& entity = entt_subsystem->GetEntity(id);
			const auto& shadow = entt_subsystem->GetRegistry()->get<ShadowCasterComponent3D>(entity);

			GPUShadowPushConstant pushConstant;
			pushConstant.vertex_buffer_address = mResourceManager->geometry_buffer()->GetVertexBufferAddress();
			pushConstant.light_space_view = shadow.lightViewProj;

			cmd.pushConstants(mShadowPipelineLayout.get(), vk::ShaderStageFlagBits::eVertex, 0, sizeof(GPUShadowPushConstant), &pushConstant);

			DrawShadowmap(cmd, mResourceManager->get_image(shadow.resourceID, GetCurrentFrameIdx()), { shadow.width, shadow.height });
		}

		cmd.end();

		return cmd;
	}

	void RenderSubystemVK::DrawShadowmap(vk::CommandBuffer cmd, const AllocatedImage& depthImage, const vk::Extent2D& shadowExtent)
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
			depthImage.image_view, vk::ImageLayout::eDepthAttachmentOptimal, vk::ResolveModeFlagBits::eNone, {},
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

	vk::CommandBuffer& RenderSubystemVK::RecordMainCommandBuffer(const RecordMainCommandBufferParams& params)
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
			params.colorImage.image_view, vk::ImageLayout::eColorAttachmentOptimal, vk::ResolveModeFlagBits::eNone, {},
			vk::ImageLayout::eUndefined, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, color_clear
		};

		vk::RenderingAttachmentInfoKHR depth_attach_info = {
			params.depthImage.image_view, vk::ImageLayout::eDepthStencilAttachmentOptimal, vk::ResolveModeFlagBits::eNone, {},
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

	void RenderSubystemVK::DrawObjects(vk::CommandBuffer cmd, const vk::Extent2D& renderExtent)
	{
		SetDrawParameters(cmd, renderExtent);

		BindBuffersAndDescriptors(cmd);

		// Make a indirect draw call for each material
		for (const auto& draw_batch : mDrawBatches)
		{
			// Use loaded material if id is valid, otherwise use default material
			if (draw_batch.matID != gInvalidID)
			{
				cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, mMaterialRegistry->get_material(draw_batch.matID).pipeline.get());
			}
			else
			{
				cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, mForwardPipeline.get());
			}

			DrawMeshBatch(cmd, draw_batch);
		}
	}

	void RenderSubystemVK::SetDrawParameters(vk::CommandBuffer cmd, const vk::Extent2D& renderExtent)
	{
		vk::Viewport viewport = {
			0, 0, static_cast<float>(renderExtent.width), static_cast<float>(renderExtent.height), 0.1f, 1.0f
		};
		cmd.setViewport(0, 1, &viewport);

		vk::Rect2D scissor = { {0, 0}, {renderExtent.width, renderExtent.height} };
		cmd.setScissor(0, 1, &scissor);
	}

	void RenderSubystemVK::BindBuffersAndDescriptors(vk::CommandBuffer cmd)
	{
		std::vector<vk::DescriptorSet> descriptors;
		descriptors.push_back(GetCurrentFrameData().objectDescriptor);
		descriptors.push_back(GetCurrentFrameData().globalDescriptor);
		descriptors.push_back(GetCurrentFrameData().textureDescriptor);
		descriptors.push_back(GetCurrentFrameData().shadowmapDescriptor);

		cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, mForwardPipelineLayout.get(), 0, descriptors.size(),
			descriptors.data(), 0, nullptr);

		
		GPUVertexShaderPushConstant push_constant_vert;
		push_constant_vert.vertex_buffer_address = mResourceManager->geometry_buffer()->GetVertexBufferAddress();

		cmd.pushConstants(mForwardPipelineLayout.get(), vk::ShaderStageFlagBits::eVertex, 0, sizeof(GPUVertexShaderPushConstant), &push_constant_vert);
		cmd.pushConstants(mForwardPipelineLayout.get(), vk::ShaderStageFlagBits::eFragment, sizeof(GPUVertexShaderPushConstant), sizeof(GPUVertexShaderPushConstant), &GetCurrentFrameData().pushConstantFrag);
		
		cmd.bindIndexBuffer(mResourceManager->geometry_buffer()->GetIndexBuffer().buffer, 0, vk::IndexType::eUint32);
	}

	void RenderSubystemVK::DrawMeshBatch(vk::CommandBuffer cmd, const MeshDrawBatch& meshDrawBatch)
	{
		vk::DeviceSize indirect_offset = meshDrawBatch.cmdIndex * sizeof(vk::DrawIndexedIndirectCommand);
		uint32_t draw_stride = sizeof(vk::DrawIndexedIndirectCommand);

		DrawIndexedIndirectCommand(cmd, GetCurrentFrameData().indirectDrawBuffer.buffer, indirect_offset,
			meshDrawBatch.cmdCount, draw_stride);
	}

	void RenderSubystemVK::DrawIndexedIndirectCommand(vk::CommandBuffer& cmd, vk::Buffer& indirectBuffer,
	                                                vk::DeviceSize offset,
	                                                uint32_t drawCount, uint32_t stride)
	{
		cmd.drawIndexedIndirect(indirectBuffer, offset, drawCount, stride);
		m_draw_calls++;
	}

	vk::CommandBuffer& RenderSubystemVK::RecordCopyCommandBuffer(uint32_t swapchainIdx)
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
			mOffscreenData.alloc_images[swapchainIdx].image, image_subresource_range
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

		cmd.blitImage(mOffscreenData.alloc_images[swapchainIdx].image, vk::ImageLayout::eTransferSrcOptimal,
		              mSwapchainData.images[swapchainIdx], vk::ImageLayout::eTransferDstOptimal, 1, &image_blit_region,
		              vk::Filter::eNearest);

		// Setup pipeline barriers for transitioning image layouts back to default

		// Offscreen Transition
		offscreen_memory_barrier = {
			vk::AccessFlagBits::eTransferRead, vk::AccessFlagBits::eNone,
			vk::ImageLayout::eTransferSrcOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, {}, {},
			mOffscreenData.alloc_images[swapchainIdx].image, image_subresource_range
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

	vk::CommandBuffer& RenderSubystemVK::RecordImguiCommandBuffer(uint32_t swapchainIdx,
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
			mSwapchainData.image_views[swapchainIdx], vk::ImageLayout::eColorAttachmentOptimal, vk::ResolveModeFlagBits::eNone, {},
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

	void RenderSubystemVK::RecordAndSubmitCommands(uint32_t swapchainIdx)
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
			params.colorImage = mOffscreenData.alloc_images[swapchainIdx];
			params.depthImage = mOffscreenData.alloc_depth_image;

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

	void RenderSubystemVK::BuildModelTransform(const Vector3f& position, const maths::Quat& orientation, const Vector3f& scale,
	                                         glm::mat4& model)
	{
		const auto scaleM = glm::scale(glm::mat4(1.0f), static_cast<glm::vec3>(scale));

		const auto orientM = glm::toMat4(static_cast<glm::quat>(orientation));

		const auto translateM = glm::translate(glm::mat4(1.0f), static_cast<glm::vec3>(position));

		model = translateM * orientM * scaleM;
	}

	bool RenderSubystemVK::LoadTexture(UUID texId, TextureDataVK& texData)
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

	void RenderSubystemVK::UnloadTexture(TextureDataVK& texData) const
	{
		mDevice.destroyImageView(texData.texture.image_view);
		mAllocator.destroyImage(texData.texture.image, texData.texture.allocation);
	}

	void RenderSubystemVK::BuildTextureDescriptorInfo(PackedVector<UUID, TextureDataVK>& textureData,
	                                                   std::vector<vk::DescriptorImageInfo>& textureImageInfos) const
	{
		textureImageInfos.clear();
		textureImageInfos.reserve(textureData.size());

		int idx = 0;
		for (auto& texData : textureData)
		{
			vk::DescriptorImageInfo textureImageInfo = {
				texData.sampler, texData.texture.image_view, vk::ImageLayout::eShaderReadOnlyOptimal
			};
			textureImageInfos.push_back(textureImageInfo);

			texData.idx = idx;
			idx++;
		}
	}

	void RenderSubystemVK::BuildShadowDescriptorInfo(std::vector<vk::DescriptorImageInfo>& shadowImageInfos)
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

			const auto& allocImage = mResourceManager->get_image(shadow.resourceID, GetCurrentFrameIdx());

			vk::DescriptorImageInfo shadowImageInfo = { mGlobalRenderData.shadowmapSampler, allocImage.image_view,
				vk::ImageLayout::eShaderReadOnlyOptimal };

			shadowImageInfos.push_back(shadowImageInfo);

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

			const auto& allocImage = mResourceManager->get_image(shadow.resourceID, GetCurrentFrameIdx());

			vk::DescriptorImageInfo shadowImageInfo = { mGlobalRenderData.shadowmapSampler, allocImage.image_view,
				vk::ImageLayout::eShaderReadOnlyOptimal };

			shadowImageInfos.push_back(shadowImageInfo);

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

			const auto& allocImage = mResourceManager->get_image(shadow.resourceID, GetCurrentFrameIdx());

			vk::DescriptorImageInfo shadowImageInfo = { mGlobalRenderData.shadowmapSampler, allocImage.image_view,
				vk::ImageLayout::eShaderReadOnlyOptimal };

			shadowImageInfos.push_back(shadowImageInfo);

			shadow.shadowIdx = idx;

			++idx;
			++d;
		}
	}

	FrameRenderData& RenderSubystemVK::GetCurrentFrameData()
	{
		return mFrameRenderData[m_frame_count % gBufferedFrames];
	}

	void RenderSubystemVK::FrameBufferResizeCallback(GLFWwindow* window, const int width, const int height)
	{
		const auto system = static_cast<RenderSubystemVK*>(glfwGetWindowUserPointer(window));

		system->mSwapchainData.resized = true;
		system->mOffscreenData.resized = true;
		system->mWindowSize.width = width;
		system->mWindowSize.height = height;
	}
}
