#include "Rendering/Vulkan/VKRenderSystem.h"

#define VMA_IMPLEMENTATION
#define VMA_DEBUG_LOG

// If you don't like the `vma::` prefix:
//#define VMA_HPP_NAMESPACE <prefix>

#include <iostream>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

#include "MathHelpers.h"
#include "VkBootstrap.h"
#include "vk_mem_alloc.h"
#include "vk_mem_alloc.hpp"
#include "glm/glm.hpp"

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"

#include "Core/Engine.h"
#include "Rendering/Vulkan/VKHelpers.h"
#include "Components/Physics/VelocityComponent.h"
#include "Window/WindowSubsystem.h"
#include "Components/SceneObjectComponent.h"
#include "Core/EnkiTSSubsystem.h"
#include "Core/SignalSubsystem.h"
#include "Assets/AssetRegistry.h"
#include "Assets/MeshAsset.h"
#include "Assets/TextureAsset.h"
#include "Components/TransformComponent.h"
#include "Components/Rendering/LightComponent.h"
#include "Components/Rendering/MeshComponent.h"
#include "ECS/EnTTSubsystem.h"
#include "Input/InputSubsystem.h"
#include "UI/Editor/UISubsystem.h"

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
	void VKRenderSystem::init()
	{
		initVulkan();

		initSwapchain(mSwapchainData, mOldSwapchainData.swapchain, mWindowSize);

		vk::Extent2D offscreenSize;
		if (mEngine->shouldRenderEditorUi())
		{
			const ImVec2 viewportSize = mEngine->uiManager()->windowViewport()->viewportSize();
			offscreenSize.width = viewportSize.x;
			offscreenSize.height = viewportSize.y;
		}
		else
		{
			offscreenSize = mWindowSize;
		}

		initOffscreen(mOffscreenData, offscreenSize, mSwapchainData.images.size());

		initCommands();

		if (mEngine->shouldRenderEditorUi())
		{
			initImGuiRenderPass();

			initSwapchainFramebuffers(mSwapchainData);
		}

		initSyncStructures();
		initBuffers();
		initSamplers();

		processComponents();
		updateRenderData();

		initDescriptors();
		initPipelines();

		if (mEngine->shouldRenderEditorUi())
		{
			initImGui();
			initOffscreenImGuiTextures(mOffscreenData);
		}

		mEditorCam.position = {0.0f, 0.0f, 15.0f};

		// Connect Signals
		const auto signalSubsystem = mEngine->getSubsystem<core::SignalSubsystem>();

		signalSubsystem->connect<input::InputEvent>(
			[&](const input::InputEvent& inputEvent)
			{
				shared_from_this()->onInputEvent(inputEvent);
			}
		);

		mIsInitialized = true;
	}

	void VKRenderSystem::render()
	{
		processEvents();

		processComponents();

		updateEditorCamera();

		updateRenderData();

		draw();

		// Clear all entity sets in mesh draw list
		for (auto& [fst, snd] : mMeshDrawList)
		{
			snd.clear();
		}
	}

	void VKRenderSystem::cleanup()
	{
		mDevice.waitIdle();

		if (mIsInitialized)
		{
			mStaticRenderData.combinedMeshBuffer.cleanup();

			for (auto texData : mTexData)
			{
				unloadTexture(texData);
			}

			mTexData.clear();

			cleanSwapchain(mSwapchainData);

			if (mOldSwapchainData.needsCleaned)
			{
				cleanSwapchain(mOldSwapchainData);
			}

			cleanOffscreen(mOffscreenData);

			if (mOldOffscreenData.needsCleaned)
			{
				cleanOffscreen(mOldOffscreenData);
			}

			mDeletionQueue.flush();

			mIsInitialized = false;
		}
	}

	void VKRenderSystem::onInputEvent(const input::InputEvent& inputEvent)
	{
		mInputEvents.push(inputEvent);
	}

	void VKRenderSystem::initVulkan()
	{
		GLFWwindow* glfwWindow = mEngine->getSubsystem<window::WindowSubsystem>()->primaryWindow();

		glfwSetWindowUserPointer(glfwWindow, this);
		glfwSetFramebufferSizeCallback(glfwWindow, frameBufferResizeCallback);

		// Create Vulkan Instance
		int width, height;
		glfwGetWindowSize(glfwWindow, &width, &height);
		mWindowSize.width = static_cast<unsigned>(width);
		mWindowSize.height = static_cast<unsigned>(height);

		vkb::InstanceBuilder instBuilder;

		auto instRet = instBuilder.set_app_name("Puffin Engine")
		                          .request_validation_layers(gEnableValidationLayers)
		                          .require_api_version(1, 3, 0)
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

		vk::PhysicalDeviceVulkan12Features physicalDevice12Features = {};
		physicalDevice12Features.descriptorIndexing = true;
		physicalDevice12Features.runtimeDescriptorArray = true;

		vkb::SystemInfo systemInfo = vkb::SystemInfo::get_system_info().value();

		// Check for desired extension support
		std::vector<const char*> device_extensions =
		{
			"VK_EXT_memory_budget",
			"VK_KHR_dynamic_rendering",
		};

		// Select GPU
		vkb::PhysicalDeviceSelector selector{vkbInst};
		vkb::PhysicalDevice physDevice = selector
		                                 .set_minimum_version(1, 3)
		                                 .set_surface(mSurface)
		                                 .set_required_features(physicalDeviceFeatures)
		                                 .set_required_features_12(physicalDevice12Features)
		                                 .add_required_extensions(device_extensions)
		                                 .select()
		                                 .value();

		// Create Vulkan Device
		vkb::DeviceBuilder deviceBuilder{physDevice};

		vk::PhysicalDeviceShaderDrawParametersFeatures shaderDrawParametersFeatures = {true};
		vk::PhysicalDeviceDynamicRenderingFeaturesKHR dynamicRenderingFeaturesKHR = {true};

		vkb::Device vkbDevice = deviceBuilder
		                        .add_pNext(&shaderDrawParametersFeatures)
		                        .add_pNext(&dynamicRenderingFeaturesKHR)
		                        .build()
		                        .value();

		mDevice = vkbDevice.device;
		mPhysicalDevice = physDevice.physical_device;

		// Get Graphics Queue
		mGraphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
		mGraphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

		// Init memory allocator
		vma::AllocatorCreateInfo allocatorInfo = {{}, mPhysicalDevice, mDevice};
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
					mIsReBarEnabled = true;
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

	void VKRenderSystem::initSwapchain(SwapchainData& swapchainData, vk::SwapchainKHR& oldSwapchain,
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
		                              .set_image_usage_flags({
			                              VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
		                              })
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

	void VKRenderSystem::initOffscreen(OffscreenData& offscreenData, const vk::Extent2D& offscreenExtent,
	                                   const int& offscreenImageCount)
	{
		offscreenData.extent = offscreenExtent;

		const vk::Extent3D imageExtent = {offscreenData.extent.width, offscreenData.extent.height, 1};

		offscreenData.imageFormat = vk::Format::eR8G8B8A8Unorm;

		const vk::ImageCreateInfo imageInfo = {
			{}, vk::ImageType::e2D, offscreenData.imageFormat, imageExtent,
			1, 1, vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
			{
				vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled |
				vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst
			}
		};

		const vk::ImageSubresourceRange subresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};

		const vk::ImageViewCreateInfo imageViewInfo = {
			{}, {}, vk::ImageViewType::e2D, offscreenData.imageFormat, {}, subresourceRange
		};

		offscreenData.allocImages.resize(offscreenImageCount);
		for (int i = 0; i < offscreenImageCount; i++)
		{
			offscreenData.allocImages[i] = util::createImage(shared_from_this(), imageInfo, imageViewInfo);
		}

		offscreenData.allocDepthImage = util::initDepthImage(shared_from_this(), imageExtent, vk::Format::eD32Sfloat);
	}

	void VKRenderSystem::initSwapchainFramebuffers(SwapchainData& swapchainData)
	{
		vk::FramebufferCreateInfo fbInfo = {
			{}, mRenderPassImGui, 1, nullptr, swapchainData.extent.width, swapchainData.extent.height, 1
		};

		// Grab number of images in swapchain
		const uint32_t swapchainImageCount = swapchainData.images.size();
		swapchainData.framebuffers.resize(swapchainImageCount);

		for (int i = 0; i < swapchainImageCount; i++)
		{
			std::array<vk::ImageView, 1> attachments = {swapchainData.imageViews[i]};

			fbInfo.pAttachments = attachments.data();
			fbInfo.attachmentCount = attachments.size();

			VK_CHECK(mDevice.createFramebuffer(&fbInfo, nullptr, &swapchainData.framebuffers[i]));
		}
	}

	void VKRenderSystem::initCommands()
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

	void VKRenderSystem::initImGuiRenderPass()
	{
		// Setup Attachments

		vk::AttachmentDescription colorAttachment =
		{
			{}, mSwapchainData.imageFormat, vk::SampleCountFlagBits::e1,
			vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
			vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
			vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR
		};

		vk::AttachmentReference colorAttachmentRef = {0, vk::ImageLayout::eColorAttachmentOptimal};

		vk::SubpassDescription subpass = {
			{}, vk::PipelineBindPoint::eGraphics, 0, nullptr,
			1, &colorAttachmentRef, {}, {}
		};

		std::array<vk::AttachmentDescription, 1> attachments = {colorAttachment};

		// Setup Dependencies
		vk::SubpassDependency colorDependency = {
			VK_SUBPASS_EXTERNAL, 0, vk::PipelineStageFlagBits::eColorAttachmentOutput,
			vk::PipelineStageFlagBits::eColorAttachmentOutput, {}, vk::AccessFlagBits::eColorAttachmentWrite
		};

		std::array<vk::SubpassDependency, 1> dependencies = {colorDependency};

		vk::RenderPassCreateInfo renderPassInfo = {
			{}, attachments.size(), attachments.data(), 1, &subpass,
			dependencies.size(), dependencies.data()
		};

		// Create Render Pass

		VK_CHECK(mDevice.createRenderPass(&renderPassInfo, nullptr, &mRenderPassImGui));

		mDeletionQueue.pushFunction([=]()
		{
			mDevice.destroyRenderPass(mRenderPassImGui);
		});
	}

	void VKRenderSystem::initSyncStructures()
	{
		vk::FenceCreateInfo fenceCreateInfo = {vk::FenceCreateFlagBits::eSignaled, nullptr};
		vk::SemaphoreCreateInfo semaphoreCreateInfo = {{}, nullptr};

		for (int i = 0; i < gBufferedFrames; i++)
		{
			VK_CHECK(mDevice.createFence(&fenceCreateInfo, nullptr, &mFrameRenderData[i].renderFence));

			VK_CHECK(mDevice.createSemaphore(&semaphoreCreateInfo, nullptr, &mFrameRenderData[i].renderSemaphore));
			VK_CHECK(mDevice.createSemaphore(&semaphoreCreateInfo, nullptr, &mFrameRenderData[i].copySemaphore));
			VK_CHECK(mDevice.createSemaphore(&semaphoreCreateInfo, nullptr, &mFrameRenderData[i].imguiSemaphore));
			VK_CHECK(mDevice.createSemaphore(&semaphoreCreateInfo, nullptr, &mFrameRenderData[i].presentSemaphore));

			mDeletionQueue.pushFunction([=]()
			{
				mDevice.destroyFence(mFrameRenderData[i].renderFence);

				mDevice.destroySemaphore(mFrameRenderData[i].renderSemaphore);
				mDevice.destroySemaphore(mFrameRenderData[i].copySemaphore);
				mDevice.destroySemaphore(mFrameRenderData[i].imguiSemaphore);
				mDevice.destroySemaphore(mFrameRenderData[i].presentSemaphore);
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

	void VKRenderSystem::initBuffers()
	{
		mStaticRenderData.combinedMeshBuffer.init(shared_from_this(), sizeof(VertexPNTV32));

		for (int i = 0; i < gBufferedFrames; i++)
		{
			// Indirect Buffer
			mFrameRenderData[i].indirectBuffer = util::createBuffer(
				mAllocator, sizeof(vk::DrawIndexedIndirectCommand) * gMaxObjects,
				vk::BufferUsageFlagBits::eIndirectBuffer | vk::BufferUsageFlagBits::eStorageBuffer |
				vk::BufferUsageFlagBits::eTransferDst,
				vma::MemoryUsage::eAuto,
				vma::AllocationCreateFlagBits::eHostAccessSequentialWrite | vma::AllocationCreateFlagBits::eMapped);

			// Global Buffers
			mFrameRenderData[i].cameraBuffer = util::createBuffer(mAllocator, sizeof(GPUCameraData),
			                                                      vk::BufferUsageFlagBits::eUniformBuffer,
			                                                      vma::MemoryUsage::eAuto,
			                                                      {
				                                                      vma::AllocationCreateFlagBits::eHostAccessSequentialWrite
				                                                      | vma::AllocationCreateFlagBits::eMapped
			                                                      });

			mFrameRenderData[i].lightBuffer = util::createBuffer(mAllocator, sizeof(GPULightData) * gMaxLightsVK,
			                                                     vk::BufferUsageFlagBits::eStorageBuffer,
			                                                     vma::MemoryUsage::eAuto,
			                                                     {
				                                                     vma::AllocationCreateFlagBits::eHostAccessSequentialWrite
				                                                     | vma::AllocationCreateFlagBits::eMapped
			                                                     });

			mFrameRenderData[i].lightStaticBuffer = util::createBuffer(mAllocator, sizeof(GPULightStaticData),
			                                                           vk::BufferUsageFlagBits::eUniformBuffer,
			                                                           vma::MemoryUsage::eAuto,
			                                                           {
				                                                           vma::AllocationCreateFlagBits::eHostAccessSequentialWrite
				                                                           | vma::AllocationCreateFlagBits::eMapped
			                                                           });

			mFrameRenderData[i].objectBuffer = util::createBuffer(mAllocator, sizeof(GPUObjectData) * gMaxObjects,
			                                                      vk::BufferUsageFlagBits::eStorageBuffer,
			                                                      vma::MemoryUsage::eAuto,
			                                                      {
				                                                      vma::AllocationCreateFlagBits::eHostAccessSequentialWrite
				                                                      | vma::AllocationCreateFlagBits::eMapped
			                                                      });

			// Material Buffers

			// Object Buffers

			mDeletionQueue.pushFunction([=]()
			{
				mAllocator.destroyBuffer(mFrameRenderData[i].objectBuffer.buffer,
				                         mFrameRenderData[i].objectBuffer.allocation);
				mAllocator.destroyBuffer(mFrameRenderData[i].lightStaticBuffer.buffer,
				                         mFrameRenderData[i].lightStaticBuffer.allocation);
				mAllocator.destroyBuffer(mFrameRenderData[i].lightBuffer.buffer,
				                         mFrameRenderData[i].lightBuffer.allocation);
				mAllocator.destroyBuffer(mFrameRenderData[i].cameraBuffer.buffer,
				                         mFrameRenderData[i].cameraBuffer.allocation);
				mAllocator.destroyBuffer(mFrameRenderData[i].indirectBuffer.buffer,
				                         mFrameRenderData[i].indirectBuffer.allocation);
			});
		}
	}

	void VKRenderSystem::initSamplers()
	{
		const vk::SamplerCreateInfo samplerInfo = {};

		mStaticRenderData.textureSampler = mDevice.createSampler(samplerInfo);

		mDeletionQueue.pushFunction([=]()
		{
			mDevice.destroySampler(mStaticRenderData.textureSampler, nullptr);
		});
	}

	void VKRenderSystem::initDescriptors()
	{
		// Descriptor Allocator/Cache

		mStaticRenderData.descriptorAllocator = std::make_shared<util::DescriptorAllocator>(mDevice);
		mStaticRenderData.descriptorLayoutCache = std::make_shared<util::DescriptorLayoutCache>(mDevice);

		for (int i = 0; i < gBufferedFrames; i++)
		{
			// Global Descriptors

			vk::DescriptorBufferInfo cameraBufferInfo = {
				mFrameRenderData[i].cameraBuffer.buffer, 0, sizeof(GPUCameraData)
			};
			vk::DescriptorBufferInfo objectBufferInfo = {
				mFrameRenderData[i].objectBuffer.buffer, 0, sizeof(GPUObjectData) * gMaxObjects
			};
			vk::DescriptorBufferInfo lightBufferInfo = {
				mFrameRenderData[i].lightBuffer.buffer, 0, sizeof(GPULightData) * gMaxLightsVK
			};
			vk::DescriptorBufferInfo lightStaticBufferInfo = {
				mFrameRenderData[i].lightStaticBuffer.buffer, 0, sizeof(GPULightStaticData)
			};

			std::vector<vk::DescriptorImageInfo> textureImageInfos;
			buildTextureDescriptorInfo(mTexData, textureImageInfos);

			util::DescriptorBuilder::begin(mStaticRenderData.descriptorLayoutCache,
			                               mStaticRenderData.descriptorAllocator)
				.bindBuffer(0, &cameraBufferInfo, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex)
				.bindBuffer(1, &objectBufferInfo, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eVertex)
				.bindBuffer(2, &lightBufferInfo, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eFragment)
				.bindBuffer(3, &lightStaticBufferInfo, vk::DescriptorType::eUniformBuffer,
				            vk::ShaderStageFlagBits::eFragment)
				.bindImages(4, textureImageInfos.size(), textureImageInfos.data(),
				            vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment)
				.build(mFrameRenderData[i].globalDescriptor, mStaticRenderData.globalSetLayout);

			// Material Descriptors
		}

		mDeletionQueue.pushFunction([=]()
		{
			mStaticRenderData.descriptorLayoutCache = nullptr;
			mStaticRenderData.descriptorAllocator = nullptr;
		});
	}

	void VKRenderSystem::initPipelines()
	{
		buildForwardRendererPipeline();
	}

	void VKRenderSystem::buildForwardRendererPipeline()
	{
		mForwardVertMod = util::ShaderModule{
			mDevice, "C:\\Projects\\PuffinEngine\\bin\\vulkan\\forward_shading\\forward_shading_vs.spv"
		};
		mForwardFragMod = util::ShaderModule{
			mDevice, "C:\\Projects\\PuffinEngine\\bin\\vulkan\\forward_shading\\forward_shading_fs.spv"
		};

		util::PipelineLayoutBuilder plb{};
		mForwardPipelineLayout = plb
		                         .descriptorSetLayout(mStaticRenderData.globalSetLayout)
		                         .createUnique(mDevice);

		vk::PipelineDepthStencilStateCreateInfo depthStencilInfo = {
			{}, true, true,
			vk::CompareOp::eLessOrEqual, false, false, {}, {}, 0.0f, 1.0f
		};

		vk::PipelineRenderingCreateInfoKHR pipelineRenderInfo = {
			0, mOffscreenData.imageFormat, mOffscreenData.allocDepthImage.format
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
		                   // Define vertex binding/attributes
		                   .vertexLayout(VertexPNTV32::getLayoutVK())
		                   // Add rendering info struct
		                   .addPNext(&pipelineRenderInfo)
		                   // Create pipeline
		                   .createUnique(mDevice, mPipelineCache, *mForwardPipelineLayout, nullptr);

		mDevice.destroyShaderModule(mForwardVertMod.module());
		mDevice.destroyShaderModule(mForwardFragMod.module());

		mDeletionQueue.pushFunction([=]()
		{
			mForwardPipelineLayout = {};
			mForwardPipeline = {};
		});
	}

	void VKRenderSystem::initImGui()
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

		vk::DescriptorPoolCreateInfo poolInfo =
		{
			vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, 1000, std::size(poolSizes), poolSizes
		};

		vk::DescriptorPool imguiPool;
		VK_CHECK(mDevice.createDescriptorPool(&poolInfo, nullptr, &imguiPool));

		// Initialize imgui for GLFW
		GLFWwindow* glfwWindow = mEngine->getSubsystem<window::WindowSubsystem>()->primaryWindow();
		ImGui_ImplGlfw_InitForVulkan(glfwWindow, true);

		// Initialize imgui for Vulkan
		ImGui_ImplVulkan_InitInfo initInfo = {
			mInstance, mPhysicalDevice, mDevice, mGraphicsQueueFamily,
			mGraphicsQueue, mPipelineCache, imguiPool,
			0, 3, 3, VK_SAMPLE_COUNT_1_BIT, nullptr
		};

		ImGui_ImplVulkan_Init(&initInfo, mRenderPassImGui);

		// Upload ImGui font textures
		util::immediateSubmit(shared_from_this(), [=](vk::CommandBuffer cmd)
		{
			ImGui_ImplVulkan_CreateFontsTexture(cmd);
		});

		// Clear font textures from cpu data
		ImGui_ImplVulkan_DestroyFontUploadObjects();

		mDeletionQueue.pushFunction([=]()
		{
			mDevice.destroyDescriptorPool(imguiPool, nullptr);
			ImGui_ImplVulkan_Shutdown();
			ImGui_ImplGlfw_Shutdown();
		});
	}

	void VKRenderSystem::initOffscreenImGuiTextures(OffscreenData& offscreenData)
	{
		offscreenData.viewportTextures.resize(offscreenData.allocImages.size());

		for (int i = 0; i < offscreenData.allocImages.size(); i++)
		{
			offscreenData.viewportTextures[i] = static_cast<ImTextureID>(ImGui_ImplVulkan_AddTexture(
				mStaticRenderData.textureSampler,
				offscreenData.allocImages[i].imageView,
				static_cast<VkImageLayout>(vk::ImageLayout::eShaderReadOnlyOptimal)));
		}
	}

	void VKRenderSystem::processEvents()
	{
		input::InputEvent inputEvent;
		while (mInputEvents.pop(inputEvent))
		{
			if (inputEvent.actionName == "CamMoveLeft")
			{
				if (inputEvent.actionState == puffin::input::KeyState::Pressed)
				{
					mMoveLeft = true;
				}
				else if (inputEvent.actionState == puffin::input::KeyState::Released)
				{
					mMoveLeft = false;
				}
			}

			if (inputEvent.actionName == "CamMoveRight")
			{
				if (inputEvent.actionState == puffin::input::KeyState::Pressed)
				{
					mMoveRight = true;
				}
				else if (inputEvent.actionState == puffin::input::KeyState::Released)
				{
					mMoveRight = false;
				}
			}

			if (inputEvent.actionName == "CamMoveForward")
			{
				if (inputEvent.actionState == puffin::input::KeyState::Pressed)
				{
					mMoveForward = true;
				}
				else if (inputEvent.actionState == puffin::input::KeyState::Released)
				{
					mMoveForward = false;
				}
			}

			if (inputEvent.actionName == "CamMoveBackward")
			{
				if (inputEvent.actionState == puffin::input::KeyState::Pressed)
				{
					mMoveBackward = true;
				}
				else if (inputEvent.actionState == puffin::input::KeyState::Released)
				{
					mMoveBackward = false;
				}
			}

			if (inputEvent.actionName == "CamMoveUp")
			{
				if (inputEvent.actionState == puffin::input::KeyState::Pressed)
				{
					mMoveUp = true;
				}
				else if (inputEvent.actionState == puffin::input::KeyState::Released)
				{
					mMoveUp = false;
				}
			}

			if (inputEvent.actionName == "CamMoveDown")
			{
				if (inputEvent.actionState == puffin::input::KeyState::Pressed)
				{
					mMoveDown = true;
				}
				else if (inputEvent.actionState == puffin::input::KeyState::Released)
				{
					mMoveDown = false;
				}
			}
		}
	}

	void VKRenderSystem::processComponents()
	{
		auto registry = mEngine->getSubsystem<ecs::EnTTSubsystem>()->registry();

		auto meshView = registry->view<const SceneObjectComponent, const TransformComponent, const MeshComponent>();

		for (auto [entity, object, transform, mesh] : meshView.each())
		{
			mMeshDrawList[mesh.meshAssetId].insert(object.id);
			mTexDrawList[mesh.textureAssetId].insert(object.id);
		}

		auto cameraView = registry->view<const SceneObjectComponent, const TransformComponent, CameraComponent>();

		for (auto [entity, object, transform, camera] : cameraView.each())
		{
			updateCameraComponent(transform, camera);
		}

		/*auto lightView = registry->view<const ECS::SceneObjectComponent, const TransformComponent, LightComponent>();
		
		for (auto [entity, object, transform, light] : lightView.each())
		{
			
		}*/
	}

	void VKRenderSystem::updateEditorCamera()
	{
		const auto inputSubsystem = mEngine->getSubsystem<input::InputSubsystem>();

		if (inputSubsystem->isCursorLocked())
		{
			// Camera Movement
			if (mMoveLeft && !mMoveRight)
			{
				mEditorCam.position += mEditorCam.right * mEditorCam.speed * mEngine->deltaTime();
			}

			if (mMoveRight && !mMoveLeft)
			{
				mEditorCam.position -= mEditorCam.right * mEditorCam.speed * mEngine->deltaTime();
			}

			if (mMoveForward && !mMoveBackward)
			{
				mEditorCam.position += mEditorCam.direction * mEditorCam.speed * mEngine->deltaTime();
			}

			if (mMoveBackward && !mMoveForward)
			{
				mEditorCam.position -= mEditorCam.direction * mEditorCam.speed * mEngine->deltaTime();
			}

			if (mMoveUp && !mMoveDown)
			{
				mEditorCam.position += mEditorCam.up * mEditorCam.speed * mEngine->deltaTime();
			}

			if (mMoveDown && !mMoveUp)
			{
				mEditorCam.position -= mEditorCam.up * mEditorCam.speed * mEngine->deltaTime();
			}

			// Mouse Rotation
			mEditorCam.yaw += inputSubsystem->getMouseXOffset();
			mEditorCam.pitch -= inputSubsystem->getMouseYOffset();

			if (mEditorCam.pitch > 89.0f)
				mEditorCam.pitch = 89.0f;

			if (mEditorCam.pitch < -89.0f)
				mEditorCam.pitch = -89.0f;

			// Calculate Direction vector from yaw and pitch of camera
			mEditorCam.direction.x = cos(maths::DegreesToRadians(mEditorCam.yaw)) * cos(
				maths::DegreesToRadians(mEditorCam.pitch));
			mEditorCam.direction.y = sin(maths::DegreesToRadians(mEditorCam.pitch));
			mEditorCam.direction.z = sin(maths::DegreesToRadians(mEditorCam.yaw)) * cos(
				maths::DegreesToRadians(mEditorCam.pitch));

			mEditorCam.direction.normalize();
		}

		// Calculate Right, Up and LookAt vectors
		mEditorCam.right = mEditorCam.up.cross(mEditorCam.direction).normalized();
		mEditorCam.lookAt = mEditorCam.position + mEditorCam.direction;

		mEditorCam.aspect = static_cast<float>(mWindowSize.width) / static_cast<float>(mWindowSize.height);

		mEditorCam.view = glm::lookAt(static_cast<glm::vec3>(mEditorCam.position),
		                              static_cast<glm::vec3>(mEditorCam.lookAt), static_cast<glm::vec3>(mEditorCam.up));

		mEditorCam.proj = glm::perspective(maths::DegreesToRadians(mEditorCam.fovY), mEditorCam.aspect,
		                                   mEditorCam.zNear, mEditorCam.zFar);
		mEditorCam.proj[1][1] *= -1;

		mEditorCam.viewProj = mEditorCam.proj * mEditorCam.view;
	}

	void VKRenderSystem::updateRenderData()
	{
		std::set<PuffinID> meshesToBeRemoved;

		for (const auto& [fst, snd] : mMeshDrawList)
		{
			const auto staticMesh = std::static_pointer_cast<assets::StaticMeshAsset>(
				assets::AssetRegistry::get()->getAsset(fst));
			mStaticRenderData.combinedMeshBuffer.addMesh(staticMesh);

			// Check if mesh is still in use by any in-flight frames
			if (!snd.empty())
			{
				getCurrentFrameData().renderedMeshes.insert(fst);
			}
			else
			{
				getCurrentFrameData().renderedMeshes.erase(fst);

				bool meshStillBeingRendered = false;
				for (const auto& frameRenderData : mFrameRenderData)
				{
					if (frameRenderData.renderedMeshes.count(fst) == 1)
						meshStillBeingRendered = true;
				}

				if (!meshStillBeingRendered)
				{
					meshesToBeRemoved.insert(fst);
				}
			}
		}

		// Remove all marked meshes from combined buffer
		if (!meshesToBeRemoved.empty())
		{
			// Currently disabled as RemoveMeshes call caused scene rendering to be messed up, will need investigated
			//m_staticRenderData.combinedMeshBuffer.RemoveMeshes(meshesToBeRemoved);

			// Remove marked meshes from draw list
			for (const auto& meshID : meshesToBeRemoved)
			{
				mMeshDrawList.erase(meshID);
			}

			meshesToBeRemoved.clear();
		}

		bool textureDescriptorNeedsUpdated = false;
		for (const auto& [fst, snd] : mTexDrawList)
		{
			if (!mTexData.contains(fst))
			{
				TextureDataVK texData;
				loadTexture(fst, texData);

				mTexData.insert(fst, texData);

				textureDescriptorNeedsUpdated = true;
			}
		}

		if (textureDescriptorNeedsUpdated)
		{
			for (int i = 0; i < gBufferedFrames; i++)
			{
				mFrameRenderData[i].textureDescriptorNeedsUpdated = true;
			}
		}
	}

	void VKRenderSystem::draw()
	{
		// Wait until GPU has finished rendering last frame. Timeout of 1 second
		VK_CHECK(mDevice.waitForFences(1, &getCurrentFrameData().renderFence, true, 1000000000));
		VK_CHECK(mDevice.resetFences(1, &getCurrentFrameData().renderFence));

		if (mEngine->shouldRenderEditorUi())
		{
			const ImVec2 viewportSize = mEngine->uiManager()->windowViewport()->viewportSize();
			if (viewportSize.x != mOffscreenData.extent.width ||
				viewportSize.y != mOffscreenData.extent.height)
			{
				mOffscreenData.resized = true;
			}
		}

		recreateSwapchain();
		recreateOffscreen();

		mDrawCalls = 0;

		uint32_t swapchainImageIdx;
		VK_CHECK(
			mDevice.acquireNextImageKHR(mSwapchainData.swapchain, 1000000000, getCurrentFrameData().presentSemaphore,
				nullptr, &swapchainImageIdx));

		// Prepare textures, scene data & indirect commands for rendering
		updateTextureDescriptors();
		prepareSceneData();
		buildIndirectCommands();

		if (mEngine->shouldRenderEditorUi())
		{
			//m_engine->GetUIManager()->DrawUI(m_engine->GetDeltaTime());

			mEngine->uiManager()->windowViewport()->draw(mOffscreenData.viewportTextures[swapchainImageIdx]);

			ImGui::Render();
		}

		recordAndSubmitCommands(swapchainImageIdx);

		mFrameNumber++;
	}

	void VKRenderSystem::updateCameraComponent(const TransformComponent& transform, CameraComponent& camera)
	{
		// Calculate Right, Up and LookAt vectors
		camera.right = camera.up.cross(transform.rotation.xyz()).normalized();
		camera.lookAt = transform.position + transform.rotation.xyz();

		camera.aspect = (float)mWindowSize.width / (float)mWindowSize.height;

		camera.view = glm::lookAt(static_cast<glm::vec3>(transform.position),
		                          static_cast<glm::vec3>(camera.lookAt), static_cast<glm::vec3>(camera.up));

		camera.proj = glm::perspective(maths::DegreesToRadians(camera.fovY), camera.aspect, camera.zNear, camera.zFar);
		camera.proj[1][1] *= -1;

		camera.viewProj = camera.proj * camera.view;
	}

	void VKRenderSystem::recreateSwapchain()
	{
		// Recreate swapchain when window is resized
		if (mSwapchainData.resized == true)
		{
			for (int i = 0; i < gBufferedFrames; i++)
			{
				mFrameRenderData[i].swapchainNeedsUpdated = true;
			}

			mOldSwapchainData = mSwapchainData;
			mOldSwapchainData.needsCleaned = true;

			initSwapchain(mSwapchainData, mOldSwapchainData.swapchain, mWindowSize);

			if (mEngine->shouldRenderEditorUi())
			{
				initSwapchainFramebuffers(mSwapchainData);
			}

			mSwapchainData.resized = false;
		}

		if (mOldSwapchainData.needsCleaned == true)
		{
			if (getCurrentFrameData().swapchainNeedsUpdated == true)
			{
				getCurrentFrameData().swapchainNeedsUpdated = false;
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
				cleanSwapchain(mOldSwapchainData);

				mOldSwapchainData.needsCleaned = false;
			}
		}
	}

	void VKRenderSystem::cleanSwapchain(SwapchainData& swapchainData)
	{
		for (int i = 0; i < swapchainData.imageViews.size(); i++)
		{
			if (mEngine->shouldRenderEditorUi())
			{
				mDevice.destroyFramebuffer(swapchainData.framebuffers[i]);
			}

			mDevice.destroyImageView(swapchainData.imageViews[i]);
		}

		mDevice.destroySwapchainKHR(swapchainData.swapchain);
	}

	void VKRenderSystem::recreateOffscreen()
	{
		if (mOffscreenData.resized == true)
		{
			for (int i = 0; i < gBufferedFrames; i++)
			{
				mFrameRenderData[i].offscreenNeedsUpdated = true;
			}

			mOldOffscreenData = mOffscreenData;
			mOldOffscreenData.needsCleaned = true;

			vk::Extent2D offscreenSize;
			if (mEngine->shouldRenderEditorUi())
			{
				const ImVec2 viewportSize = mEngine->uiManager()->windowViewport()->viewportSize();
				offscreenSize.width = static_cast<uint32_t>(viewportSize.x);
				offscreenSize.height = static_cast<uint32_t>(viewportSize.y);
			}
			else
			{
				offscreenSize = mWindowSize;
			}

			initOffscreen(mOffscreenData, offscreenSize, mSwapchainData.images.size());

			if (mEngine->shouldRenderEditorUi())
			{
				initOffscreenImGuiTextures(mOffscreenData);
			}

			mOffscreenData.resized = false;
		}

		if (mOldOffscreenData.needsCleaned == true)
		{
			if (getCurrentFrameData().offscreenNeedsUpdated == true)
			{
				getCurrentFrameData().offscreenNeedsUpdated = false;
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
				cleanOffscreen(mOldOffscreenData);

				mOldOffscreenData.needsCleaned = false;
			}
		}
	}

	void VKRenderSystem::cleanOffscreen(OffscreenData& offscreenData)
	{
		mDevice.destroyImageView(offscreenData.allocDepthImage.imageView);
		mAllocator.destroyImage(offscreenData.allocDepthImage.image, offscreenData.allocDepthImage.allocation);

		for (int i = 0; i < offscreenData.allocImages.size(); i++)
		{
			if (mEngine->shouldRenderEditorUi())
			{
				ImGui_ImplVulkan_RemoveTexture(static_cast<VkDescriptorSet>(offscreenData.viewportTextures[i]));
			}

			mDevice.destroyImageView(offscreenData.allocImages[i].imageView);
			mAllocator.destroyImage(offscreenData.allocImages[i].image, offscreenData.allocImages[i].allocation);
		}
	}

	void VKRenderSystem::updateTextureDescriptors()
	{
		if (mIsInitialized && getCurrentFrameData().textureDescriptorNeedsUpdated)
		{
			std::vector<vk::DescriptorImageInfo> textureImageInfos;
			buildTextureDescriptorInfo(mTexData, textureImageInfos);

			util::DescriptorBuilder::begin(mStaticRenderData.descriptorLayoutCache,
			                               mStaticRenderData.descriptorAllocator)
				.updateImages(4, textureImageInfos.size(), textureImageInfos.data(),
				              vk::DescriptorType::eCombinedImageSampler)
				.update(getCurrentFrameData().globalDescriptor);

			getCurrentFrameData().textureDescriptorNeedsUpdated = false;
		}
	}

	void VKRenderSystem::prepareSceneData()
	{
		// Prepare camera data
		const AllocatedBuffer& cameraBuffer = getCurrentFrameData().cameraBuffer;

		GPUCameraData camUBO = {};
		camUBO.proj = mEditorCam.proj;
		camUBO.view = mEditorCam.view;
		camUBO.viewProj = mEditorCam.viewProj;

		memcpy(cameraBuffer.allocInfo.pMappedData, &camUBO, sizeof(GPUCameraData));

		// Prepare object data
		prepareObjectData();

		// Prepare light data
		prepareLightData();
	}

	void VKRenderSystem::prepareObjectData()
	{
		const auto enkiTSSubSystem = mEngine->getSubsystem<core::EnkiTSSubsystem>();

		const AllocatedBuffer& objectBuffer = getCurrentFrameData().objectBuffer;

		std::vector<GPUObjectData> objects = {};
		objects.reserve(gMaxObjects);

		// Calculate t value for rendering interpolated position
		const double t = mEngine->accumulatedTime() / mEngine->timeStepFixed();

		std::vector<PuffinID> entities;
		entities.reserve(gMaxObjects);

		int numObjects = 0;

		for (const auto& [fst, snd] : mMeshDrawList)
		{
			for (const auto entityID : snd)
			{
				entities.emplace_back(entityID);

				numObjects++;
			}
		}

		objects.resize(numObjects);

		const uint32_t numThreads = enkiTSSubSystem->getTaskScheduler()->GetNumTaskThreads();

		std::vector<std::vector<std::pair<GPUObjectData, uint32_t>>> threadObjects;
		// Temp object vectors for writing to by threads

		threadObjects.resize(numThreads);
		for (int idx = 0; idx < threadObjects.size(); idx++)
		{
			threadObjects[idx].reserve(500);
		}

		auto enttSubsystem = mEngine->getSubsystem<ecs::EnTTSubsystem>();
		auto registry = enttSubsystem->registry();

		enki::TaskSet task(numObjects, [&](enki::TaskSetPartition range, uint32_t threadnum)
		{
			uint32_t numObjectsForThread = range.end - range.start;

			for (uint32_t objectIdx = range.start; objectIdx < range.end; objectIdx++)
			{
				auto entity = enttSubsystem->getEntity(entities[objectIdx]);

				const auto& transform = registry->get<TransformComponent>(entity);
				const auto& mesh = registry->get<MeshComponent>(entity);

#ifdef PFN_USE_DOUBLE_PRECISION
				Vector3d position = {0.0};
#else
				Vector3f position = { 0.0f };
#endif

				if (registry->all_of<physics::VelocityComponent>(entity))
				{
					const auto& velocity = registry->get<physics::VelocityComponent>(entity);

#ifdef PFN_USE_DOUBLE_PRECISION
					Vector3d interpolatedPosition = transform.position + velocity.linear * mEngine->timeStepFixed();
#else
					Vector3f interpolatedPosition = transform.position + velocity.linear * m_engine->GetTimeStep();
#endif

					position = maths::Lerp(transform.position, interpolatedPosition, t);
				}
				else
				{
					position = transform.position;
				}

				GPUObjectData object;

				buildModelTransform(position, transform.rotation.eulerAnglesRad(), transform.scale, object.model);
				object.texIndex = mTexData[mesh.textureAssetId].idx;

				threadObjects[threadnum].emplace_back(object, objectIdx);
			}
		});

		task.m_MinRange = 500; // Try and ensure each thread gets

		enkiTSSubSystem->getTaskScheduler()->AddTaskSetToPipe(&task);

		enkiTSSubSystem->getTaskScheduler()->WaitforTask(&task);

		for (const auto& tempThreadObjects : threadObjects)
		{
			for (const auto& [fst, snd] : tempThreadObjects)
			{
				objects[snd] = fst;
			}
		}

		const auto* objectData = objects.data();

		std::copy_n(objectData, numObjects, static_cast<GPUObjectData*>(objectBuffer.allocInfo.pMappedData));
	}

	void VKRenderSystem::prepareLightData()
	{
		const AllocatedBuffer& lightBuffer = getCurrentFrameData().lightBuffer;

		auto* lightSSBO = static_cast<GPULightData*>(lightBuffer.allocInfo.pMappedData);

		int i = 0;

		auto registry = mEngine->getSubsystem<ecs::EnTTSubsystem>()->registry();

		auto lightView = registry->view<const SceneObjectComponent, const TransformComponent, const LightComponent>();

		for (auto [entity, object, transform, light] : lightView.each())
		{
			// Break out of loop of maximum number of lights has been reached
			if (i >= gMaxLightsVK)
			{
				break;
			}

			lightSSBO[i].position = static_cast<glm::vec3>(transform.position);
			lightSSBO[i].direction = static_cast<glm::vec3>(transform.rotation.xyz());
			lightSSBO[i].color = static_cast<glm::vec3>(light.color);
			lightSSBO[i].ambientSpecular = glm::vec3(light.ambientIntensity, light.specularIntensity,
			                                         light.specularExponent);
			lightSSBO[i].attenuation = glm::vec3(light.constantAttenuation, light.linearAttenuation,
			                                     light.quadraticAttenuation);
			lightSSBO[i].cutoffAngle = glm::vec3(
				glm::cos(glm::radians(light.innerCutoffAngle)),
				glm::cos(glm::radians(light.outerCutoffAngle)), 0.0f);
			lightSSBO[i].type = static_cast<int>(light.type);

			i++;
		}

		// Prepare light static data
		const AllocatedBuffer& lightStaticBuffer = getCurrentFrameData().lightStaticBuffer;

		GPULightStaticData lightStaticUBO = {};
		lightStaticUBO.numLights = i;
		lightStaticUBO.viewPos = static_cast<glm::vec3>(mEditorCam.position);

		memcpy(lightStaticBuffer.allocInfo.pMappedData, &lightStaticUBO, sizeof(GPULightStaticData));
	}

	void VKRenderSystem::buildIndirectCommands()
	{
		AllocatedBuffer& indirectBuffer = getCurrentFrameData().indirectBuffer;

		std::vector<vk::DrawIndexedIndirectCommand> indirectCmds = {};
		indirectCmds.resize(gMaxObjects);

		int idx = 0;

		auto enttSubsystem = mEngine->getSubsystem<ecs::EnTTSubsystem>();
		auto registry = enttSubsystem->registry();

		for (const auto [fst, snd] : mMeshDrawList)
		{
			for (const auto entityID : snd)
			{
				auto entity = enttSubsystem->getEntity(entityID);

				const auto& mesh = registry->get<MeshComponent>(entity);

				indirectCmds[idx].vertexOffset = mStaticRenderData.combinedMeshBuffer.
				                                                   meshVertexOffset(mesh.meshAssetId);
				indirectCmds[idx].firstIndex = mStaticRenderData.combinedMeshBuffer.meshIndexOffset(mesh.meshAssetId);
				indirectCmds[idx].indexCount = mStaticRenderData.combinedMeshBuffer.meshIndexCount(mesh.meshAssetId);
				indirectCmds[idx].firstInstance = idx;
				indirectCmds[idx].instanceCount = 1;

				idx++;
			}
		}

		getCurrentFrameData().drawCount = idx;

		const auto* indirectData = indirectCmds.data();

		std::copy_n(indirectData, idx,
		            static_cast<vk::DrawIndexedIndirectCommand*>(indirectBuffer.allocInfo.pMappedData));
	}

	vk::CommandBuffer VKRenderSystem::recordMainCommandBuffer(const uint32_t& swapchainIdx,
	                                                          const vk::Extent2D& renderExtent, const AllocatedImage&
	                                                          colorImage, const AllocatedImage& depthImage)
	{
		vk::CommandBuffer cmd = getCurrentFrameData().mainCommandBuffer;

		// Reset command buffer for recording new commands
		cmd.reset();

		// Begin command buffer execution
		vk::CommandBufferBeginInfo cmdBeginInfo = {
			vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
			nullptr, nullptr
		};

		VK_CHECK(cmd.begin(&cmdBeginInfo));

		// Transition color image to color attachment optimal
		vk::ImageSubresourceRange imageSubresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};

		vk::ImageMemoryBarrier offscreenMemoryBarrierToColor = {
			vk::AccessFlagBits::eNone, vk::AccessFlagBits::eColorAttachmentWrite,
			vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal, {}, {},
			colorImage.image, imageSubresourceRange
		};

		cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eColorAttachmentOutput,
		                    {}, 0, nullptr, 0, nullptr,
		                    1, &offscreenMemoryBarrierToColor);

		vk::ClearValue clearValue;
		clearValue.color = {0.0f, 0.7f, 0.9f, 1.0f};

		vk::ClearValue depthClear;
		depthClear.depthStencil.depth = 1.f;

		std::array<vk::ClearValue, 2> clearValues = {clearValue, depthClear};

		// Begin Rendering
		vk::RenderingAttachmentInfoKHR colorAttachInfo = {
			colorImage.imageView, vk::ImageLayout::eColorAttachmentOptimal, vk::ResolveModeFlagBits::eNone, {},
			vk::ImageLayout::eUndefined, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, clearValue
		};

		vk::RenderingAttachmentInfoKHR depthAttachInfo = {
			depthImage.imageView, vk::ImageLayout::eDepthStencilAttachmentOptimal, vk::ResolveModeFlagBits::eNone, {},
			vk::ImageLayout::eUndefined, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, depthClear
		};

		vk::RenderingInfoKHR renderInfo = {
			{}, vk::Rect2D{{0, 0}, renderExtent}, 1, {}, 1, &colorAttachInfo, &depthAttachInfo
		};

		cmd.beginRendering(&renderInfo);

		drawObjects(cmd, renderExtent);

		// End Rendering
		cmd.endRendering();

		// Transition layout to Shader Read Optimal
		vk::ImageMemoryBarrier offscreenMemoryBarrierToShader = {
			vk::AccessFlagBits::eColorAttachmentWrite, vk::AccessFlagBits::eNone,
			vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, {}, {},
			colorImage.image, imageSubresourceRange
		};

		cmd.pipelineBarrier(vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eBottomOfPipe,
		                    {}, 0, nullptr, 0, nullptr,
		                    1, &offscreenMemoryBarrierToShader);

		// Finish command buffer recording
		cmd.end();

		return cmd;
	}

	void VKRenderSystem::drawObjects(vk::CommandBuffer cmd, const vk::Extent2D& renderExtent)
	{
		cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, mForwardPipeline.get());

		vk::Viewport viewport = {
			0, 0, static_cast<float>(renderExtent.width), static_cast<float>(renderExtent.height), 0.1f, 1.0f
		};
		cmd.setViewport(0, 1, &viewport);

		vk::Rect2D scissor = {{0, 0}, {renderExtent.width, renderExtent.height}};
		cmd.setScissor(0, 1, &scissor);

		cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, mForwardPipelineLayout.get(), 0, 1,
		                       &getCurrentFrameData().globalDescriptor, 0, nullptr);

		cmd.bindVertexBuffers(0, mStaticRenderData.combinedMeshBuffer.vertexBuffer().buffer, {0});
		cmd.bindIndexBuffer(mStaticRenderData.combinedMeshBuffer.indexBuffer().buffer, 0, vk::IndexType::eUint32);

		vk::DeviceSize indirectOffset = 0;
		uint32_t drawStride = sizeof(vk::DrawIndexedIndirectCommand);

		drawIndexedIndirectCommand(cmd, getCurrentFrameData().indirectBuffer.buffer, indirectOffset,
		                           getCurrentFrameData().drawCount, drawStride);
	}

	void VKRenderSystem::drawIndexedIndirectCommand(vk::CommandBuffer& cmd, vk::Buffer& indirectBuffer,
	                                                vk::DeviceSize offset,
	                                                uint32_t drawCount, uint32_t stride)
	{
		cmd.drawIndexedIndirect(indirectBuffer, offset, getCurrentFrameData().drawCount, stride);
		mDrawCalls++;
	}

	vk::CommandBuffer VKRenderSystem::recordCopyCommandBuffer(uint32_t swapchainIdx)
	{
		vk::CommandBuffer cmd = getCurrentFrameData().copyCommandBuffer;

		// Reset command buffer for recording new commands
		cmd.reset();

		// Begin command buffer execution
		vk::CommandBufferBeginInfo cmdBeginInfo = {
			vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
			nullptr, nullptr
		};

		VK_CHECK(cmd.begin(&cmdBeginInfo));

		// Setup pipeline barriers for transitioning image layouts

		vk::ImageSubresourceRange imageSubresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};

		// Offscreen Transition
		vk::ImageMemoryBarrier offscreenMemoryBarrier = {
			vk::AccessFlagBits::eNone, vk::AccessFlagBits::eTransferRead,
			vk::ImageLayout::eShaderReadOnlyOptimal, vk::ImageLayout::eTransferSrcOptimal, {}, {},
			mOffscreenData.allocImages[swapchainIdx].image, imageSubresourceRange
		};

		cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer,
		                    {}, 0, nullptr, 0, nullptr,
		                    1, &offscreenMemoryBarrier);

		// Swapchain Transition
		vk::ImageMemoryBarrier swapchainMemoryBarrier = {
			vk::AccessFlagBits::eMemoryRead, vk::AccessFlagBits::eTransferWrite,
			vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, {}, {},
			mSwapchainData.images[swapchainIdx], imageSubresourceRange
		};

		cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer,
		                    {}, 0, nullptr, 0, nullptr,
		                    1, &swapchainMemoryBarrier);

		// Blit (Copy with auto format coversion (RGB to BGR)) offscreen to swapchain image
		vk::Offset3D blitSize =
		{
			static_cast<int32_t>(mOffscreenData.extent.width),
			static_cast<int32_t>(mOffscreenData.extent.height),
			1
		};

		std::array<vk::Offset3D, 2> offsets = {};
		offsets[1] = blitSize;

		vk::ImageBlit imageBlitRegion =
		{
			{vk::ImageAspectFlagBits::eColor, 0, 0, 1}, offsets,
			{vk::ImageAspectFlagBits::eColor, 0, 0, 1}, offsets
		};

		cmd.blitImage(mOffscreenData.allocImages[swapchainIdx].image, vk::ImageLayout::eTransferSrcOptimal,
		              mSwapchainData.images[swapchainIdx], vk::ImageLayout::eTransferDstOptimal, 1, &imageBlitRegion,
		              vk::Filter::eNearest);

		// Setup pipeline barriers for transitioning image layouts back to default

		// Offscreen Transition
		offscreenMemoryBarrier = {
			vk::AccessFlagBits::eTransferRead, vk::AccessFlagBits::eNone,
			vk::ImageLayout::eTransferSrcOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, {}, {},
			mOffscreenData.allocImages[swapchainIdx].image, imageSubresourceRange
		};

		cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer,
		                    {}, 0, nullptr, 0, nullptr,
		                    1, &offscreenMemoryBarrier);

		// Swapchain Transition
		swapchainMemoryBarrier = {
			vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eMemoryRead,
			vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::ePresentSrcKHR, {}, {},
			mSwapchainData.images[swapchainIdx], imageSubresourceRange
		};

		cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer,
		                    {}, 0, nullptr, 0, nullptr,
		                    1, &swapchainMemoryBarrier);

		cmd.end();

		return cmd;
	}

	vk::CommandBuffer VKRenderSystem::recordImGuiCommandBuffer(uint32_t swapchainIdx, const vk::Extent2D& renderExtent,
	                                                           vk::Framebuffer framebuffer)
	{
		vk::CommandBuffer cmd = getCurrentFrameData().imguiCommandBuffer;

		// Reset command buffer for recording new commands
		cmd.reset();

		// Begin command buffer execution
		vk::CommandBufferBeginInfo cmdBeginInfo = {
			vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
			nullptr, nullptr
		};

		VK_CHECK(cmd.begin(&cmdBeginInfo));

		vk::ClearValue clearValue;
		clearValue.color = {1.0f, 1.0f, 1.0f, 1.0f};

		std::array<vk::ClearValue, 1> clearValues = {clearValue};

		// Begin main renderpass
		vk::RenderPassBeginInfo rpInfo = {
			mRenderPassImGui, framebuffer,
			vk::Rect2D{{0, 0}, renderExtent}, clearValues.size(), clearValues.data(), nullptr
		};

		cmd.beginRenderPass(&rpInfo, vk::SubpassContents::eInline);

		// Record Imgui Draw Data and draw functions into command buffer
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

		cmd.endRenderPass();

		cmd.end();

		return cmd;
	}

	void VKRenderSystem::recordAndSubmitCommands(uint32_t swapchainIdx)
	{
		// Record command buffers
		vk::CommandBuffer mainCmd = recordMainCommandBuffer(swapchainIdx, mOffscreenData.extent,
		                                                    mOffscreenData.allocImages[swapchainIdx],
		                                                    mOffscreenData.allocDepthImage);

		// Submit all commands
		std::vector<vk::CommandBuffer> commands = {mainCmd};

		// Prepare submission to queue
		vk::PipelineStageFlags waitStage = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
		vk::SubmitInfo renderSubmit =
		{
			1, &getCurrentFrameData().presentSemaphore,
			&waitStage, static_cast<uint32_t>(commands.size()), commands.data(),
			1, &getCurrentFrameData().renderSemaphore, nullptr
		};

		std::vector submits = {renderSubmit};

		if (mEngine->shouldRenderEditorUi())
		{
			vk::CommandBuffer imguiCmd = recordImGuiCommandBuffer(swapchainIdx, mSwapchainData.extent,
			                                                      mSwapchainData.framebuffers[swapchainIdx]);

			vk::SubmitInfo imguiSubmit =
			{
				1, &getCurrentFrameData().renderSemaphore,
				&waitStage, 1, &imguiCmd,
				1, &getCurrentFrameData().imguiSemaphore, nullptr
			};

			submits.push_back(imguiSubmit);
		}
		else
		{
			vk::CommandBuffer copyCmd = recordCopyCommandBuffer(swapchainIdx);

			vk::SubmitInfo copySubmit =
			{
				1, &getCurrentFrameData().renderSemaphore,
				&waitStage, 1, &copyCmd,
				1, &getCurrentFrameData().copySemaphore, nullptr
			};

			submits.push_back(copySubmit);
		}

		VK_CHECK(mGraphicsQueue.submit(submits.size(), submits.data(), getCurrentFrameData().renderFence));

		vk::Semaphore waitSemaphore;

		if (mEngine->shouldRenderEditorUi())
		{
			waitSemaphore = getCurrentFrameData().imguiSemaphore;
		}
		else
		{
			waitSemaphore = getCurrentFrameData().copySemaphore;
		}

		vk::PresentInfoKHR presentInfo =
		{
			1, &waitSemaphore, 1, &mSwapchainData.swapchain, &swapchainIdx
		};

		VK_CHECK(mGraphicsQueue.presentKHR(&presentInfo));
	}

	void VKRenderSystem::buildModelTransform(const Vector3f& position, const Vector3f& rotation, const Vector3f& scale,
	                                         glm::mat4& outModel) const
	{
		// Set Translation
		outModel = glm::translate(glm::mat4(1.0f), (glm::vec3)position);

		// Set Rotation
		outModel = glm::rotate(outModel, rotation.z, glm::vec3(0.0f, 0.0f, -1.0f));
		outModel = glm::rotate(outModel, rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
		outModel = glm::rotate(outModel, rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));

		// Set Scale
		outModel = glm::scale(outModel, (glm::vec3)scale);
	}

	bool VKRenderSystem::loadMesh(PuffinID meshId, MeshDataVK& meshData)
	{
		if (const auto meshAsset = std::static_pointer_cast<assets::StaticMeshAsset>(
			assets::AssetRegistry::get()->getAsset(meshId)); meshAsset && meshAsset->load())
		{
			meshData.assetId = meshId;

			meshData.numVertices = meshAsset->numVertices();
			meshData.numIndices = meshAsset->numIndices();

			meshData.vertexBuffer = util::initVertexBuffer(shared_from_this(), meshAsset->vertices().data(),
			                                               meshAsset->numVertices(), meshAsset->vertexSize());

			meshData.indexBuffer = util::initIndexBuffer(shared_from_this(), meshAsset->indices().data(),
			                                             meshAsset->numIndices(), meshAsset->indexSize());

			meshAsset->unload();

			return true;
		}
		else
		{
			return false;
		}
	}

	void VKRenderSystem::unloadMesh(MeshDataVK& meshData) const
	{
		mAllocator.destroyBuffer(meshData.vertexBuffer.buffer, meshData.vertexBuffer.allocation);
		mAllocator.destroyBuffer(meshData.indexBuffer.buffer, meshData.indexBuffer.allocation);
	}

	bool VKRenderSystem::loadTexture(PuffinID texId, TextureDataVK& texData)
	{
		if (const auto texAsset = std::static_pointer_cast<assets::TextureAsset>(
			assets::AssetRegistry::get()->getAsset(texId)); texAsset && texAsset->load())
		{
			texData.assetId = texId;

			texData.sampler = mStaticRenderData.textureSampler;

			texData.texture = util::initTexture(shared_from_this(), texAsset->pixelData(),
			                                    texAsset->textureWidth(), texAsset->textureHeight(),
			                                    texAsset->textureSizePerPixel(),
			                                    gTexFormatVK.at(texAsset->textureFormat()));

			texAsset->unload();

			return true;
		}
		else
		{
			return false;
		}
	}

	void VKRenderSystem::unloadTexture(TextureDataVK& texData) const
	{
		mDevice.destroyImageView(texData.texture.imageView);
		mAllocator.destroyImage(texData.texture.image, texData.texture.allocation);
	}

	void VKRenderSystem::buildTextureDescriptorInfo(PackedVector<TextureDataVK>& texData,
	                                                std::vector<vk::DescriptorImageInfo>& textureImageInfos) const
	{
		textureImageInfos.clear();
		textureImageInfos.reserve(mTexData.size());

		int idx = 0;
		for (auto& texData : texData)
		{
			vk::DescriptorImageInfo textureImageInfo = {
				texData.sampler, texData.texture.imageView, vk::ImageLayout::eShaderReadOnlyOptimal
			};
			textureImageInfos.push_back(textureImageInfo);

			texData.idx = idx;
			idx++;
		}
	}
}
