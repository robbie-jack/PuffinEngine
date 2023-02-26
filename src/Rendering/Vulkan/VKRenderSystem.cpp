#include "Rendering/Vulkan/VKRenderSystem.hpp"

#define VMA_IMPLEMENTATION

// If you don't like the `vma::` prefix:
//#define VMA_HPP_NAMESPACE <prefix>

#include <iostream>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

#include "MathHelpers.h"
#include "vk_mem_alloc.h"
#include "vk_mem_alloc.hpp"
#include "VkBootstrap.h"
#include "glm/glm.hpp"

#include "Window/WindowSubsystem.hpp"
#include "Engine/Engine.hpp"
#include "Rendering/Vulkan/VKHelpers.hpp"
#include "Components/Rendering/MeshComponent.h"
#include "Assets/AssetRegistry.h"
#include "Assets/MeshAsset.h"
#include "Assets/TextureAsset.h"

#include "Components/TransformComponent.h"
#include "Components/Rendering/LightComponent.h"
#include "Engine/SignalSubsystem.hpp"
#include "Input/InputSubsystem.h"

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

namespace Puffin::Rendering::VK
{
	void VKRenderSystem::Init()
	{
		// Register Components
		m_world->RegisterComponent<CameraMatComponent>();
		m_world->AddComponentDependencies<CameraComponent, CameraMatComponent>();

		InitVulkan();
		InitSwapchain(m_swapchainData, m_oldSwapchainData.swapchain);
		InitCommands();
		InitDefaultRenderPass();
		InitSwapchainFramebuffers(m_swapchainData);
		InitSyncStructures();
		InitBuffers();
		InitSamplers();

		ProcessComponents();
		UpdateRenderData();

		InitDescriptors();
		InitPipelines();

		m_editorCam.position = { 0.0f, 0.0f, 15.0f };

		// Connect Signals
		const auto signalSubsystem = m_engine->GetSubsystem<Core::SignalSubsystem>();

		signalSubsystem->Connect<Input::InputEvent>(
			[&](const Input::InputEvent& inputEvent)
			{
				shared_from_this()->OnInputEvent(inputEvent);
			}
		);

		m_isInitialized = true;
	}

	void VKRenderSystem::Update()
	{
		ProcessEvents();

		ProcessComponents();

		UpdateEditorCamera();

		UpdateRenderData();

		Draw();

		m_meshDrawList.clear();
	}

	void VKRenderSystem::Cleanup()
	{
		m_device.waitIdle();

		if (m_isInitialized)
		{
			for (auto meshData : m_meshData)
			{
				UnloadMesh(meshData);
			}

			m_meshData.Clear();

			for (auto texData : m_texData)
			{
				UnloadTexture(texData);
			}

			m_texData.Clear();

			CleanSwapchain(m_swapchainData);

			if (m_oldSwapchainData.needsCleaned)
			{
				CleanSwapchain(m_oldSwapchainData);
			}

			m_deletionQueue.Flush();

			m_isInitialized = false;
		}
	}

	void VKRenderSystem::OnInputEvent(const Input::InputEvent& inputEvent)
	{
		m_inputEvents.Push(inputEvent);
	}

	void VKRenderSystem::InitVulkan()
	{
		GLFWwindow* glfwWindow = m_engine->GetSubsystem<Window::WindowSubsystem>()->GetPrimaryWindow();

		glfwSetWindowUserPointer(glfwWindow, this);
		glfwSetFramebufferSizeCallback(glfwWindow, FrameBufferResizeCallback);

		// Create Vulkan Instance
		int width, height;
		glfwGetWindowSize(glfwWindow, &width, &height);
		m_windowSize.width = static_cast<unsigned>(width);
		m_windowSize.height = static_cast<unsigned>(height);

		vkb::InstanceBuilder instBuilder;

		auto instRet = instBuilder.set_app_name("Puffin Engine")
			.request_validation_layers(enableValidationLayers)
			.require_api_version(1, 3, 0)
			.use_default_debug_messenger()
			.build();

		vkb::Instance vkbInst = instRet.value();

		// Store Instance/Debug Messenger
		m_instance = vkbInst.instance;
		m_debugMessenger = vkbInst.debug_messenger;

		// Get Window Surface
		VkSurfaceKHR surface;
		glfwCreateWindowSurface(m_instance, glfwWindow, nullptr, &surface);
		m_surface = surface;

		vk::PhysicalDeviceFeatures physicalDeviceFeatures = {};
		physicalDeviceFeatures.shaderSampledImageArrayDynamicIndexing = true;

		vk::PhysicalDeviceVulkan12Features physicalDevice12Features = {};
		physicalDevice12Features.descriptorIndexing = true;
		physicalDevice12Features.runtimeDescriptorArray = true;

		// Select GPU
		vkb::PhysicalDeviceSelector selector { vkbInst };
		vkb::PhysicalDevice physDevice = selector
			.set_minimum_version(1, 3)
			.set_surface(m_surface)
			.set_required_features(physicalDeviceFeatures)
			.set_required_features_12(physicalDevice12Features)
			.select()
			.value();

		// Create Vulkan Device
		vkb::DeviceBuilder deviceBuilder { physDevice };

		vk::PhysicalDeviceShaderDrawParametersFeatures shaderDrawParametersFeatures = { true };

		vkb::Device vkbDevice = deviceBuilder
			.add_pNext(&shaderDrawParametersFeatures)
			.build()
			.value();

		m_device = vkbDevice.device;
		m_physicalDevice = physDevice.physical_device;

		// Get Graphics Queue
		m_graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
		m_graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

		// Init memory allocator
		vma::AllocatorCreateInfo allocatorInfo = { {}, m_physicalDevice, m_device};
		allocatorInfo.instance = m_instance;

		VK_CHECK(vma::createAllocator(&allocatorInfo, &m_allocator));

		m_deletionQueue.PushFunction([=]()
		{
			m_allocator.destroy();

			m_device.destroy();
			m_instance.destroySurfaceKHR(m_surface);
			vkb::destroy_debug_utils_messenger(m_instance, m_debugMessenger);
			m_instance.destroy();
		});
	}

	void VKRenderSystem::InitSwapchain(SwapchainData& swapchainData, vk::SwapchainKHR& oldSwapchain)
	{
		vkb::SwapchainBuilder swapchainBuilder { m_physicalDevice, m_device, m_surface};

		vkb::Swapchain vkbSwapchain = swapchainBuilder
			.use_default_format_selection()
			// Vsync present mode
			.set_old_swapchain(oldSwapchain)
			.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
			.set_desired_extent(m_windowSize.width, m_windowSize.height)
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

		// Create Swapchain Depth Image
		vk::Extent3D depthExtent = { m_windowSize.width, m_windowSize.height, 1 };

		swapchainData.depthImage = Util::InitDepthImage(shared_from_this(), depthExtent, vk::Format::eD32Sfloat);
	}

	void VKRenderSystem::InitCommands()
	{
		vk::CommandPoolCreateInfo commandPoolInfo = { vk::CommandPoolCreateFlagBits::eResetCommandBuffer, m_graphicsQueueFamily };
		vk::CommandBufferAllocateInfo commandBufferInfo = { {}, vk::CommandBufferLevel::ePrimary, 1 };

		// Init Main Command Pools/Buffers
		for (int i = 0; i < G_BUFFERED_FRAMES; i++)
		{
			VK_CHECK(m_device.createCommandPool(&commandPoolInfo, nullptr, &m_frameRenderData[i].commandPool));

			commandBufferInfo.commandPool = m_frameRenderData[i].commandPool;
			VK_CHECK(m_device.allocateCommandBuffers(&commandBufferInfo, &m_frameRenderData[i].mainCommandBuffer));

			m_deletionQueue.PushFunction([=]()
			{
				m_device.destroyCommandPool(m_frameRenderData[i].commandPool);
			});
		}

		// Init Upload Context Command Pool/Buffer
		commandPoolInfo = { vk::CommandPoolCreateFlagBits::eResetCommandBuffer, m_graphicsQueueFamily };
		VK_CHECK(m_device.createCommandPool(&commandPoolInfo, nullptr, &m_uploadContext.commandPool));

		commandBufferInfo = { m_uploadContext.commandPool, vk::CommandBufferLevel::ePrimary, 1 };
		VK_CHECK(m_device.allocateCommandBuffers(&commandBufferInfo, &m_uploadContext.commandBuffer));

		m_deletionQueue.PushFunction([=]()
		{
			m_device.destroyCommandPool(m_uploadContext.commandPool);
		});
	}

	void VKRenderSystem::InitDefaultRenderPass()
	{
		// Setup Attachments

		vk::AttachmentDescription colorAttachment = 
		{
			{}, m_swapchainData.imageFormat, vk::SampleCountFlagBits::e1,
			vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
			vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
			vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR
		};

		vk::AttachmentReference colorAttachmentRef = { 0, vk::ImageLayout::eColorAttachmentOptimal };

		vk::AttachmentDescription depthAttachment =
		{
			{}, m_swapchainData.depthImage.format, vk::SampleCountFlagBits::e1,
			vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
			vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eDontCare,
			vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal
		};

		vk::AttachmentReference depthAttachRef = { 1, vk::ImageLayout::eDepthStencilAttachmentOptimal };

		vk::SubpassDescription subpass = { {}, vk::PipelineBindPoint::eGraphics, 0, nullptr,
			1, &colorAttachmentRef, {}, & depthAttachRef };

		std::array<vk::AttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };

		// Setup Dependencies
		vk::SubpassDependency colorDependency = { VK_SUBPASS_EXTERNAL, 0, vk::PipelineStageFlagBits::eColorAttachmentOutput,
			vk::PipelineStageFlagBits::eColorAttachmentOutput, {}, vk::AccessFlagBits::eColorAttachmentWrite };

		vk::SubpassDependency depthDependecy = { VK_SUBPASS_EXTERNAL, 0,
			{ vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests },
			{ vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests },
			{}, vk::AccessFlagBits::eDepthStencilAttachmentWrite };

		std::array<vk::SubpassDependency, 2> dependencies = { colorDependency, depthDependecy };

		vk::RenderPassCreateInfo renderPassInfo = { {}, attachments.size(), attachments.data(), 1, &subpass,
			dependencies.size(), dependencies.data() };

		// Create Render Pass

		VK_CHECK(m_device.createRenderPass(&renderPassInfo, nullptr, &m_renderPass));

		m_deletionQueue.PushFunction([=]()
		{
			m_device.destroyRenderPass(m_renderPass);
		});
	}

	void VKRenderSystem::InitSwapchainFramebuffers(SwapchainData& swapchainData)
	{
		vk::FramebufferCreateInfo fbInfo = { {}, m_renderPass, 1, nullptr, m_windowSize.width, m_windowSize.height, 1 };

		// Grab number of images in swapchain
		const uint32_t swapchainImageCount = swapchainData.images.size();
		swapchainData.framebuffers.resize(swapchainImageCount);

		for (int i = 0; i < swapchainImageCount; i++)
		{
			std::array<vk::ImageView, 2> attachments = { swapchainData.imageViews[i], swapchainData.depthImage.imageView };

			fbInfo.pAttachments = attachments.data();
			fbInfo.attachmentCount = attachments.size();

			VK_CHECK(m_device.createFramebuffer(&fbInfo, nullptr, &swapchainData.framebuffers[i]));
		}
	}

	void VKRenderSystem::InitSyncStructures()
	{
		vk::FenceCreateInfo fenceCreateInfo = { vk::FenceCreateFlagBits::eSignaled, nullptr };
		vk::SemaphoreCreateInfo semaphoreCreateInfo = { {}, nullptr };

		for (int i = 0; i < G_BUFFERED_FRAMES; i++)
		{
			VK_CHECK(m_device.createFence(&fenceCreateInfo, nullptr, &m_frameRenderData[i].renderFence));

			VK_CHECK(m_device.createSemaphore(&semaphoreCreateInfo, nullptr, &m_frameRenderData[i].presentSemaphore));
			VK_CHECK(m_device.createSemaphore(&semaphoreCreateInfo, nullptr, &m_frameRenderData[i].renderSemaphore));

			m_deletionQueue.PushFunction([=]()
			{
				m_device.destroyFence(m_frameRenderData[i].renderFence);

				m_device.destroySemaphore(m_frameRenderData[i].presentSemaphore);
				m_device.destroySemaphore(m_frameRenderData[i].renderSemaphore);
			});
		}

		// Init Upload Context Fence
		fenceCreateInfo = vk::FenceCreateInfo{ {}, nullptr };
		VK_CHECK(m_device.createFence(&fenceCreateInfo, nullptr, &m_uploadContext.uploadFence));

		m_deletionQueue.PushFunction([=]()
		{
			m_device.destroyFence(m_uploadContext.uploadFence);
		});
	}

	void VKRenderSystem::InitBuffers()
	{
		for (int i = 0; i < G_BUFFERED_FRAMES; i++)
		{
			// Global Buffers
			m_frameRenderData[i].cameraBuffer = Util::CreateBuffer(m_allocator, sizeof(GPUCameraData),
				vk::BufferUsageFlagBits::eUniformBuffer, vma::MemoryUsage::eAuto, vma::AllocationCreateFlagBits::eHostAccessSequentialWrite);

			m_frameRenderData[i].objectBuffer = Util::CreateBuffer(m_allocator, sizeof(GPUObjectData) * G_MAX_OBJECTS,
				vk::BufferUsageFlagBits::eStorageBuffer, vma::MemoryUsage::eAuto, vma::AllocationCreateFlagBits::eHostAccessSequentialWrite);

			m_frameRenderData[i].lightBuffer = Util::CreateBuffer(m_allocator, sizeof(GPULightData) * G_MAX_LIGHTS,
				vk::BufferUsageFlagBits::eStorageBuffer, vma::MemoryUsage::eAuto, vma::AllocationCreateFlagBits::eHostAccessSequentialWrite);

			m_frameRenderData[i].lightStaticBuffer = Util::CreateBuffer(m_allocator, sizeof(GPULightStaticData),
				vk::BufferUsageFlagBits::eUniformBuffer, vma::MemoryUsage::eAuto, vma::AllocationCreateFlagBits::eHostAccessSequentialWrite);

			// Material Buffers

			// Object Buffers

			m_deletionQueue.PushFunction([=]()
			{
				m_allocator.destroyBuffer(m_frameRenderData[i].lightStaticBuffer.buffer, m_frameRenderData[i].lightStaticBuffer.allocation);
				m_allocator.destroyBuffer(m_frameRenderData[i].lightBuffer.buffer, m_frameRenderData[i].lightBuffer.allocation);
				m_allocator.destroyBuffer(m_frameRenderData[i].objectBuffer.buffer, m_frameRenderData[i].objectBuffer.allocation);
				m_allocator.destroyBuffer(m_frameRenderData[i].cameraBuffer.buffer, m_frameRenderData[i].cameraBuffer.allocation);
			});
		}
	}

	void VKRenderSystem::InitSamplers()
	{
		const vk::SamplerCreateInfo samplerInfo = {};

		m_staticRenderData.textureSampler = m_device.createSampler(samplerInfo);

		m_deletionQueue.PushFunction([=]()
		{
			m_device.destroySampler(m_staticRenderData.textureSampler, nullptr);
		});
	}

	void VKRenderSystem::InitDescriptors()
	{
		// Descriptor Allocator/Cache

		m_staticRenderData.descriptorAllocator = std::make_shared<Util::DescriptorAllocator>(m_device);
		m_staticRenderData.descriptorLayoutCache = std::make_shared<Util::DescriptorLayoutCache>(m_device);

		for (int i = 0; i < G_BUFFERED_FRAMES; i++)
		{
			// Global Descriptors

			vk::DescriptorBufferInfo cameraBufferInfo = { m_frameRenderData[i].cameraBuffer.buffer, 0, sizeof(GPUCameraData) };
			vk::DescriptorBufferInfo objectBufferInfo = { m_frameRenderData[i].objectBuffer.buffer, 0, sizeof(GPUObjectData) * G_MAX_OBJECTS };
			vk::DescriptorBufferInfo lightBufferInfo = { m_frameRenderData[i].lightBuffer.buffer, 0, sizeof(GPULightData) * G_MAX_LIGHTS };
			vk::DescriptorBufferInfo lightStaticBufferInfo = { m_frameRenderData[i].lightStaticBuffer.buffer, 0, sizeof(GPULightStaticData) };

			std::vector<vk::DescriptorImageInfo> textureImageInfos;
			BuildTextureDescriptorInfo(m_texData, textureImageInfos);

			Util::DescriptorBuilder::Begin(m_staticRenderData.descriptorLayoutCache, m_staticRenderData.descriptorAllocator)
				.BindBuffer(0, &cameraBufferInfo, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex)
				.BindBuffer(1, &objectBufferInfo, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eVertex)
				.BindBuffer(2, &lightBufferInfo, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eFragment)
				.BindBuffer(3, &lightStaticBufferInfo, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eFragment)
				.BindImages(4, textureImageInfos.size(), textureImageInfos.data(), 
					vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment)
				.Build(m_frameRenderData[i].globalDescriptor, m_staticRenderData.globalSetLayout);

			// Material Descriptors

		}

		m_deletionQueue.PushFunction([=]()
		{
			m_staticRenderData.descriptorLayoutCache = nullptr;
			m_staticRenderData.descriptorAllocator = nullptr;
		});
	}

	void VKRenderSystem::InitPipelines()
	{
		BuildForwardRendererPipeline();
	}

	void VKRenderSystem::BuildForwardRendererPipeline()
	{
		m_forwardVertMod = Util::ShaderModule{ m_device, "C:\\Projects\\PuffinEngine\\bin\\vulkan\\forward_shading\\forward_shading_vs.spv" };
		m_forwardFragMod = Util::ShaderModule{ m_device, "C:\\Projects\\PuffinEngine\\bin\\vulkan\\forward_shading\\forward_shading_fs.spv" };

		Util::PipelineLayoutBuilder plb{};
		m_forwardPipelineLayout = plb
			.DescriptorSetLayout(m_staticRenderData.globalSetLayout)
			.CreateUnique(m_device);

		vk::PipelineDepthStencilStateCreateInfo depthStencilInfo = { {}, true, true,
			vk::CompareOp::eLessOrEqual, false, false, {}, {}, 0.0f, 1.0f };

		Util::PipelineBuilder pb{ m_windowSize.width, m_windowSize.height };
		m_forwardPipeline = pb
			// Define dynamic state which can change each frame (currently viewport and scissor size)
			.DynamicState(vk::DynamicState::eViewport)
			.DynamicState(vk::DynamicState::eScissor)
			// Define vertex/fragment shaders
			.Shader(vk::ShaderStageFlagBits::eVertex, m_forwardVertMod)
			.Shader(vk::ShaderStageFlagBits::eFragment, m_forwardFragMod)
			.DepthStencilState(depthStencilInfo)
			// Define vertex binding/attributes
			.VertexLayout(VertexPNTV32::GetLayoutVK())
			// Create pipeline
			.CreateUnique(m_device, m_pipelineCache, *m_forwardPipelineLayout, m_renderPass);

		m_device.destroyShaderModule(m_forwardVertMod.Module());
		m_device.destroyShaderModule(m_forwardFragMod.Module());

		m_deletionQueue.PushFunction([=]()
		{
			m_forwardPipelineLayout = {};
			m_forwardPipeline = {};
		});
	}

	void VKRenderSystem::ProcessEvents()
	{
		Input::InputEvent inputEvent;
		while (m_inputEvents.Pop(inputEvent))
		{
			if (inputEvent.actionName == "CamMoveLeft")
			{
				if (inputEvent.actionState == Puffin::Input::KeyState::PRESSED)
				{
					m_moveLeft = true;
				}
				else if (inputEvent.actionState == Puffin::Input::KeyState::RELEASED)
				{
					m_moveLeft = false;
				}
			}

			if (inputEvent.actionName == "CamMoveRight")
			{
				if (inputEvent.actionState == Puffin::Input::KeyState::PRESSED)
				{
					m_moveRight = true;
				}
				else if (inputEvent.actionState == Puffin::Input::KeyState::RELEASED)
				{
					m_moveRight = false;
				}
			}

			if (inputEvent.actionName == "CamMoveForward")
			{
				if (inputEvent.actionState == Puffin::Input::KeyState::PRESSED)
				{
					m_moveForward = true;
				}
				else if (inputEvent.actionState == Puffin::Input::KeyState::RELEASED)
				{
					m_moveForward = false;
				}
			}

			if (inputEvent.actionName == "CamMoveBackward")
			{
				if (inputEvent.actionState == Puffin::Input::KeyState::PRESSED)
				{
					m_moveBackward = true;
				}
				else if (inputEvent.actionState == Puffin::Input::KeyState::RELEASED)
				{
					m_moveBackward = false;
				}
			}

			if (inputEvent.actionName == "CamMoveUp")
			{
				if (inputEvent.actionState == Puffin::Input::KeyState::PRESSED)
				{
					m_moveUp = true;
				}
				else if (inputEvent.actionState == Puffin::Input::KeyState::RELEASED)
				{
					m_moveUp = false;
				}
			}

			if (inputEvent.actionName == "CamMoveDown")
			{
				if (inputEvent.actionState == Puffin::Input::KeyState::PRESSED)
				{
					m_moveDown = true;
				}
				else if (inputEvent.actionState == Puffin::Input::KeyState::RELEASED)
				{
					m_moveDown = false;
				}
			}
		}
	}

	void VKRenderSystem::ProcessComponents()
	{
		std::vector<std::shared_ptr<ECS::Entity>> meshEntities;
		ECS::GetEntities<TransformComponent, MeshComponent>(m_world, meshEntities);
		for (const auto& entity : meshEntities)
		{
			const auto& mesh = entity->GetComponent<MeshComponent>();

			if (m_meshDrawList.count(mesh.meshAssetID) == 0)
			{
				m_meshDrawList.insert({mesh.meshAssetID, std::set<ECS::EntityID>()});
			}

			m_meshDrawList[mesh.meshAssetID].insert(entity->ID());

			if (m_texDrawList.count(mesh.textureAssetID) == 0)
			{
				m_texDrawList.insert({mesh.textureAssetID, std::set<ECS::EntityID>()});
			}

			m_texDrawList[mesh.textureAssetID].insert(entity->ID());
		}

		std::vector<std::shared_ptr<ECS::Entity>> camEntities;
		ECS::GetEntities<TransformComponent, CameraComponent, CameraMatComponent>(m_world, camEntities);
		for (const auto& entity : camEntities)
		{
			UpdateCameraComponent(entity);
		}

		std::vector<std::shared_ptr<ECS::Entity>> lightEntities;
		ECS::GetEntities<TransformComponent, LightComponent>(m_world, lightEntities);
		for (const auto& entity : lightEntities)
		{
			
		}
	}

	void VKRenderSystem::UpdateEditorCamera()
	{
		const auto inputSubsystem = m_engine->GetSubsystem<Input::InputSubsystem>();

		if (inputSubsystem->IsCursorLocked())
		{
			// Camera Movement
			if (m_moveLeft && !m_moveRight)
			{
				m_editorCam.position += m_editorCam.right * m_editorCam.speed * m_engine->GetDeltaTime();
			}

			if (m_moveRight && !m_moveLeft)
			{
				m_editorCam.position -= m_editorCam.right * m_editorCam.speed * m_engine->GetDeltaTime();
			}

			if (m_moveForward && !m_moveBackward)
			{
				m_editorCam.position += m_editorCam.direction * m_editorCam.speed * m_engine->GetDeltaTime();
			}

			if (m_moveBackward && !m_moveForward)
			{
				m_editorCam.position -= m_editorCam.direction * m_editorCam.speed * m_engine->GetDeltaTime();
			}

			if (m_moveUp && !m_moveDown)
			{
				m_editorCam.position += m_editorCam.up * m_editorCam.speed * m_engine->GetDeltaTime();
			}

			if (m_moveDown && !m_moveUp)
			{
				m_editorCam.position -= m_editorCam.up * m_editorCam.speed * m_engine->GetDeltaTime();
			}

			// Mouse Rotation
			m_editorCam.yaw += inputSubsystem->GetMouseXOffset();
			m_editorCam.pitch -= inputSubsystem->GetMouseYOffset();

			if (m_editorCam.pitch > 89.0f)
				m_editorCam.pitch = 89.0f;

			if (m_editorCam.pitch < -89.0f)
				m_editorCam.pitch = -89.0f;

			// Calculate Direction vector from yaw and pitch of camera
			m_editorCam.direction.x = cos(Maths::DegreesToRadians(m_editorCam.yaw)) * cos(Maths::DegreesToRadians(m_editorCam.pitch));
			m_editorCam.direction.y = sin(Maths::DegreesToRadians(m_editorCam.pitch));
			m_editorCam.direction.z = sin(Maths::DegreesToRadians(m_editorCam.yaw)) * cos(Maths::DegreesToRadians(m_editorCam.pitch));

			m_editorCam.direction.Normalise();
		}

		// Calculate Right, Up and LookAt vectors
		m_editorCam.right = m_editorCam.up.Cross(m_editorCam.direction).Normalised();
		m_editorCam.lookat = m_editorCam.position + m_editorCam.direction;

		m_editorCam.aspect = static_cast<float>(m_windowSize.width) / static_cast<float>(m_windowSize.height);

		m_editorCamMats.view = glm::lookAt(static_cast<glm::vec3>(m_editorCam.position),
			static_cast<glm::vec3>(m_editorCam.lookat), static_cast<glm::vec3>(m_editorCam.up));

		m_editorCamMats.proj = glm::perspective(Maths::DegreesToRadians(m_editorCam.fovY), m_editorCam.aspect, m_editorCam.zNear, m_editorCam.zFar);
		m_editorCamMats.proj[1][1] *= -1;

		m_editorCamMats.viewProj = m_editorCamMats.proj * m_editorCamMats.view;
	}

	void VKRenderSystem::UpdateRenderData()
	{
		for (const auto [fst, snd] : m_meshDrawList)
		{
			if (!m_meshData.Contains(fst))
			{
				MeshData meshData;
				LoadMesh(fst, meshData);

				m_meshData.Insert(fst, meshData);
			}
		}

		bool textureDescriptorNeedsUpdated = false;
		for (const auto [fst, snd] : m_texDrawList)
		{
			if (!m_texData.Contains(fst))
			{
				TextureData texData;
				LoadTexture(fst, texData);

				m_texData.Insert(fst, texData);

				textureDescriptorNeedsUpdated = true;
			}
		}

		if (textureDescriptorNeedsUpdated)
		{
			for (int i = 0; i < G_BUFFERED_FRAMES; i++)
			{
				m_frameRenderData[i].textureDescriptorNeedsupdated = true;
			}
		}
	}

	void VKRenderSystem::Draw()
	{
		// Wait until GPU has finished rendering last frame. Timeout of 1 second
		VK_CHECK(m_device.waitForFences(1, &GetCurrentFrameData().renderFence, true, 1000000000));
		VK_CHECK(m_device.resetFences(1, &GetCurrentFrameData().renderFence));

		RecreateSwapchain();

		uint32_t swapchainImageIdx;
		VK_CHECK(m_device.acquireNextImageKHR(m_swapchainData.swapchain, 1000000000, GetCurrentFrameData().presentSemaphore, nullptr, &swapchainImageIdx));

		vk::CommandBuffer cmd = GetCurrentFrameData().mainCommandBuffer;

		// Reset command buffer for recording new commands
		cmd.reset();

		UpdateTextureDescriptors();
		PrepareSceneData();

		// Begin command buffer execution
		vk::CommandBufferBeginInfo cmdBeginInfo = { vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
			nullptr, nullptr };

		VK_CHECK(cmd.begin(&cmdBeginInfo));

		vk::ClearValue clearValue;
		clearValue.color = { 0.0f, 0.7f, 0.9f, 1.0f };

		vk::ClearValue depthClear;
		depthClear.depthStencil.depth = 1.f;

		std::array<vk::ClearValue, 2> clearValues = { clearValue, depthClear };

		// Begin main renderpass
		vk::RenderPassBeginInfo rpInfo = { m_renderPass, m_swapchainData.framebuffers[swapchainImageIdx],
			vk::Rect2D{ {0, 0}, m_windowSize }, clearValues.size(), clearValues.data(), nullptr };

		cmd.beginRenderPass(&rpInfo, vk::SubpassContents::eInline);

		DrawObjects(cmd);

		// End main renderpass
		cmd.endRenderPass();

		// Finish command buffer recording
		cmd.end();

		// Prepare submission to queue
		vk::PipelineStageFlags waitStage = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
		vk::SubmitInfo submit = 
		{
			1, & GetCurrentFrameData().presentSemaphore,
			&waitStage, 1, &cmd,
			1, & GetCurrentFrameData().renderSemaphore, nullptr
		};

		VK_CHECK(m_graphicsQueue.submit(1, &submit, GetCurrentFrameData().renderFence));

		vk::PresentInfoKHR presentInfo =
		{
			1, &GetCurrentFrameData().renderSemaphore, 1, &m_swapchainData.swapchain, &swapchainImageIdx
		};

		VK_CHECK(m_graphicsQueue.presentKHR(&presentInfo));

		m_frameNumber++;
	}

	void VKRenderSystem::UpdateCameraComponent(std::shared_ptr<ECS::Entity> entity)
	{
		auto& transform = entity->GetComponent<TransformComponent>();
		auto& cam = entity->GetComponent<CameraComponent>();
		auto& camMats = entity->GetComponent<CameraMatComponent>();

		// Calculate Right, Up and LookAt vectors
		cam.right = cam.up.Cross(cam.direction).Normalised();
		cam.lookat = transform.position + cam.direction;

		cam.aspect = (float)m_windowSize.width / (float)m_windowSize.height;

		camMats.view = glm::lookAt(static_cast<glm::vec3>(transform.position), 
			static_cast<glm::vec3>(cam.lookat), static_cast<glm::vec3>(cam.up));

		camMats.proj = glm::perspective(Maths::DegreesToRadians(cam.fovY), cam.aspect, cam.zNear, cam.zFar);
		camMats.proj[1][1] *= -1;

		camMats.viewProj = camMats.proj * camMats.view;
	}

	void VKRenderSystem::RecreateSwapchain()
	{
		// Recreate swapchain when window is resized
		if (m_windowResized)
		{
			for (int i = 0; i < G_BUFFERED_FRAMES; i++)
			{
				m_frameRenderData[i].swapchainNeedsUpdated = true;
			}

			m_oldSwapchainData = m_swapchainData;

			InitSwapchain(m_swapchainData, m_oldSwapchainData.swapchain);
			InitSwapchainFramebuffers(m_swapchainData);

			m_oldSwapchainData.needsCleaned = true;

			m_windowResized = false;
		}

		if (m_oldSwapchainData.needsCleaned == true)
		{
			if (GetCurrentFrameData().swapchainNeedsUpdated == true)
			{
				GetCurrentFrameData().swapchainNeedsUpdated = false;
			}

			int framesUsingNewSwapchain = 0;
			for (int i = 0; i < G_BUFFERED_FRAMES; i++)
			{
				if (m_frameRenderData[i].swapchainNeedsUpdated == false)
				{
					framesUsingNewSwapchain++;
				}
			}

			if (framesUsingNewSwapchain == G_BUFFERED_FRAMES)
			{
				CleanSwapchain(m_oldSwapchainData);

				m_oldSwapchainData.needsCleaned = false;
			}
		}
	}

	void VKRenderSystem::CleanSwapchain(SwapchainData& swapchainData)
	{
		for (int i = 0; i < swapchainData.framebuffers.size(); i++)
		{
			m_device.destroyFramebuffer(swapchainData.framebuffers[i]);
		}

		m_device.destroyImageView(swapchainData.depthImage.imageView);
		m_allocator.destroyImage(swapchainData.depthImage.image, swapchainData.depthImage.allocation);

		for (int i = 0; i < swapchainData.imageViews.size(); i++)
		{
			m_device.destroyImageView(swapchainData.imageViews[i]);
		}

		m_device.destroySwapchainKHR(swapchainData.swapchain);
	}

	void VKRenderSystem::UpdateTextureDescriptors()
	{
		if (m_isInitialized && GetCurrentFrameData().textureDescriptorNeedsupdated)
		{
			std::vector<vk::DescriptorImageInfo> textureImageInfos;
			BuildTextureDescriptorInfo(m_texData, textureImageInfos);

			Util::DescriptorBuilder::Begin(m_staticRenderData.descriptorLayoutCache, m_staticRenderData.descriptorAllocator)
				.UpdateImages(4, textureImageInfos.size(), textureImageInfos.data(),
					vk::DescriptorType::eCombinedImageSampler)
				.Update(GetCurrentFrameData().globalDescriptor);

			GetCurrentFrameData().textureDescriptorNeedsupdated = false;
		}
	}

	void VKRenderSystem::PrepareSceneData()
	{
		// Prepare camera data

		GPUCameraData camUBO = {};
		camUBO.proj = m_editorCamMats.proj;
		camUBO.view = m_editorCamMats.view;
		camUBO.viewProj = m_editorCamMats.viewProj;

		void* camData;
		VK_CHECK(m_allocator.mapMemory(GetCurrentFrameData().cameraBuffer.allocation, &camData));

		memcpy(camData, &camUBO, sizeof(GPUCameraData));

		m_allocator.unmapMemory(GetCurrentFrameData().cameraBuffer.allocation);

		// Prepare object data

		void* objectData;
		VK_CHECK(m_allocator.mapMemory(GetCurrentFrameData().objectBuffer.allocation, &objectData));

		auto* objectSSBO = static_cast<GPUObjectData*>(objectData);

		int i = 0;

		for (const auto [fst, snd] : m_meshDrawList)
		{
			for (const auto entityID : snd)
			{
				const auto& transform = m_world->GetComponent<TransformComponent>(entityID);
				const auto& mesh = m_world->GetComponent<MeshComponent>(entityID);

				objectSSBO[i].model = BuildModelTransform(transform.position, transform.rotation, transform.scale);
				objectSSBO[i].invModel = glm::inverse(objectSSBO[i].model);
				objectSSBO[i].texIndex = m_texData[mesh.textureAssetID].idx;

				i++;
			}
		}

		m_allocator.unmapMemory(GetCurrentFrameData().objectBuffer.allocation);

		// Prepare light data

		void* lightData;
		VK_CHECK(m_allocator.mapMemory(GetCurrentFrameData().lightBuffer.allocation, &lightData));

		auto* lightSSBO = static_cast<GPULightData*>(lightData);

		i = 0;

		std::vector<std::shared_ptr<ECS::Entity>> lightEntities;
		ECS::GetEntities<TransformComponent, LightComponent>(m_world, lightEntities);
		for (const auto& entity : lightEntities)
		{
			// Break out of loop of maximum number of lights has been reached
			if (i >= G_MAX_LIGHTS)
			{
				break;
			}

			const auto& transform = entity->GetComponent<TransformComponent>();
			const auto& light = entity->GetComponent<LightComponent>();

			lightSSBO[i].position = static_cast<glm::vec3>(transform.position);
			lightSSBO[i].direction = static_cast<glm::vec3>(light.direction);
			lightSSBO[i].color = static_cast<glm::vec3>(light.color);
			lightSSBO[i].ambientSpecular = glm::vec3(light.ambientIntensity, light.specularIntensity, light.specularExponent);
			lightSSBO[i].attenuation = glm::vec3(light.constantAttenuation, light.linearAttenuation, light.quadraticAttenuation);
			lightSSBO[i].cutoffAngle = glm::vec3(light.innerCutoffAngle, light.outerCutoffAngle, 0.0f);
			lightSSBO[i].type = static_cast<int>(light.type);

			i++;
		}

		m_allocator.unmapMemory(GetCurrentFrameData().lightBuffer.allocation);

		// Prepare light static data

		GPULightStaticData lightStaticUBO = {};
		lightStaticUBO.numLights = i;
		lightStaticUBO.viewPos = static_cast<glm::vec3>(m_editorCam.position);

		void* lightStaticData;
		VK_CHECK(m_allocator.mapMemory(GetCurrentFrameData().lightStaticBuffer.allocation, &lightStaticData));

		memcpy(lightStaticData, &lightStaticUBO, sizeof(GPULightStaticData));

		m_allocator.unmapMemory(GetCurrentFrameData().lightStaticBuffer.allocation);
	}

	void VKRenderSystem::DrawObjects(vk::CommandBuffer cmd)
	{
		cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_forwardPipeline.get());

		vk::Viewport viewport = { 0, 0, static_cast<float>(m_windowSize.width), static_cast<float>(m_windowSize.height), 0.1f, 1.0f };
		cmd.setViewport(0, 1, &viewport);

		vk::Rect2D scissor = { { 0, 0 }, { m_windowSize.width, m_windowSize.height } };
		cmd.setScissor(0, 1, &scissor);

		cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_forwardPipelineLayout.get(), 0, 1, &GetCurrentFrameData().globalDescriptor, 0, nullptr);

		int i = 0;
		for (const auto [fst, snd] : m_meshDrawList)
		{
			cmd.bindVertexBuffers(0, m_meshData[fst].vertexBuffer.buffer, { 0 });
			cmd.bindIndexBuffer(m_meshData[fst].indexBuffer.buffer, 0, vk::IndexType::eUint32);

			for (const auto entityID : snd)
			{
				cmd.drawIndexed(m_meshData[fst].numIndices, 1, 0, 0, i);

				i++;
			}
		}
	}

	glm::mat4 VKRenderSystem::BuildModelTransform(const Vector3f& position, const Vector3f& rotation, const Vector3f& scale)
	{
		// Set Translation
		glm::mat4 model_transform = glm::translate(glm::mat4(1.0f), (glm::vec3)position);

		// Set Rotation
		model_transform = glm::rotate(model_transform, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
		model_transform = glm::rotate(model_transform, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
		model_transform = glm::rotate(model_transform, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, -1.0f));

		// Set Scale
		model_transform = glm::scale(model_transform, (glm::vec3)scale);

		return model_transform;
	}

	bool VKRenderSystem::LoadMesh(UUID meshID, MeshData& meshData)
	{
		const auto meshAsset = std::static_pointer_cast<Assets::StaticMeshAsset>(Assets::AssetRegistry::Get()->GetAsset(meshID));

		if (meshAsset && meshAsset->Load())
		{
			meshData.assetID = meshID;

			meshData.numVertices = meshAsset->GetNumVertices();
			meshData.numIndices = meshAsset->GetNumIndices();

			meshData.vertexBuffer = Util::InitVertexBuffer(shared_from_this(), meshAsset->GetVertices().data(),
				meshAsset->GetNumVertices(), meshAsset->GetVertexSize());

			meshData.indexBuffer = Util::InitIndexBuffer(shared_from_this(), meshAsset->GetIndices().data(),
				meshAsset->GetNumIndices(), meshAsset->GetIndexSize());

			meshAsset->Unload();

			return true;
		}
		else
		{
			return false;
		}
	}

	void VKRenderSystem::UnloadMesh(MeshData& meshData) const
	{
		m_allocator.destroyBuffer(meshData.vertexBuffer.buffer, meshData.vertexBuffer.allocation);
		m_allocator.destroyBuffer(meshData.indexBuffer.buffer, meshData.indexBuffer.allocation);
	}

	bool VKRenderSystem::LoadTexture(UUID texID, TextureData& texData)
	{
		const auto texAsset = std::static_pointer_cast<Assets::TextureAsset>(Assets::AssetRegistry::Get()->GetAsset(texID));

		if (texAsset && texAsset->Load())
		{
			texData.assetID = texID;

			texData.sampler = m_staticRenderData.textureSampler;

			texData.texture = Util::InitTexture(shared_from_this(), texAsset->GetPixelData(), 
				texAsset->GetTextureWidth(), texAsset->GetTextureHeight(), 
				texAsset->GetTexturePixelSize(), g_texFormatMap.at(texAsset->GetTextureFormat()));

			texAsset->Unload();

			return true;
		}
		else
		{
			return false;
		}
	}

	void VKRenderSystem::UnloadTexture(TextureData& texData) const
	{
		m_device.destroyImageView(texData.texture.imageView);
		m_allocator.destroyImage(texData.texture.image, texData.texture.allocation);
	}

	void VKRenderSystem::BuildTextureDescriptorInfo(PackedVector<TextureData>& texData, std::vector<vk::DescriptorImageInfo>& textureImageInfos) const
	{
		textureImageInfos.clear();
		textureImageInfos.reserve(m_texData.Size());

		int idx = 0;
		for (auto& texData : texData)
		{
			vk::DescriptorImageInfo textureImageInfo = { texData.sampler, texData.texture.imageView, vk::ImageLayout::eShaderReadOnlyOptimal };
			textureImageInfos.push_back(textureImageInfo);

			texData.idx = idx;
			idx++;
		}
	}
}
