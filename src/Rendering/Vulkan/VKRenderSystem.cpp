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

#include "Components/TransformComponent.h"
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
		InitVulkan();
		InitSwapchain();
		InitCommands();
		InitDefaultRenderPass();
		InitFramebuffers();
		InitSyncStructures();
		InitBuffers();
		InitDescriptors();
		InitPipelines();

		// Register Components
		m_world->RegisterComponent<CameraMatComponent>();
		m_world->AddComponentDependencies<CameraComponent, CameraMatComponent>();

		ProcessComponents();
		UpdateRenderData();

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

		// Select GPU
		vkb::PhysicalDeviceSelector selector { vkbInst };
		vkb::PhysicalDevice physDevice = selector
			.set_minimum_version(1, 3)
			.set_surface(m_surface)
			.select()
			.value();

		// Create Vulkan Device
		vkb::DeviceBuilder deviceBuilder { physDevice };

		vk::PhysicalDeviceShaderDrawParametersFeatures shaderDrawParametersFeatures = { true };

		vkb::Device vkbDevice = deviceBuilder.add_pNext(&shaderDrawParametersFeatures).build().value();

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

	void VKRenderSystem::InitSwapchain()
	{
		vkb::SwapchainBuilder swapchainBuilder { m_physicalDevice, m_device, m_surface};

		vkb::Swapchain vkbSwapchain = swapchainBuilder
			.use_default_format_selection()
			// Vsync present mode
			.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
			.set_desired_extent(m_windowSize.width, m_windowSize.height)
			.build()
			.value();

		m_swapchain = vkbSwapchain.swapchain;
		m_swapchainImageFormat = static_cast<vk::Format>(vkbSwapchain.image_format);

		// Grab Images
		std::vector<VkImage> images = vkbSwapchain.get_images().value();
		std::vector<VkImageView> imageViews = vkbSwapchain.get_image_views().value();

		m_swapchainImages.resize(images.size());
		m_swapchainImageViews.resize(imageViews.size());

		for (int i = 0; i < images.size(); i++)
		{
			m_swapchainImages[i] = static_cast<vk::Image>(images[i]);
			m_swapchainImageViews[i] = static_cast<vk::ImageView>(imageViews[i]);
		}

		images.clear();
		imageViews.clear();

		// Create Swapchain Depth Image
		vk::Extent3D depthExtent = { m_windowSize.width, m_windowSize.height, 1 };

		m_swapchainDepthImage = Util::InitDepthImage(shared_from_this(), depthExtent, vk::Format::eD32Sfloat);

		m_deletionQueue.PushFunction([=]()
		{
			m_device.destroyImageView(m_swapchainDepthImage.imageView);
			m_allocator.destroyImage(m_swapchainDepthImage.image, m_swapchainDepthImage.allocation);

			for (int i = 0; i < m_swapchainImageViews.size(); i++)
			{
				m_device.destroyImageView(m_swapchainImageViews[i]);
			}

			m_device.destroySwapchainKHR(m_swapchain);
		});
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
			{}, m_swapchainImageFormat, vk::SampleCountFlagBits::e1,
			vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
			vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
			vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR
		};

		vk::AttachmentReference colorAttachmentRef = { 0, vk::ImageLayout::eColorAttachmentOptimal };

		vk::AttachmentDescription depthAttachment =
		{
			{}, m_swapchainDepthImage.format, vk::SampleCountFlagBits::e1,
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

	void VKRenderSystem::InitFramebuffers()
	{
		vk::FramebufferCreateInfo fbInfo = { {}, m_renderPass, 1, nullptr, m_windowSize.width, m_windowSize.height, 1 };

		// Grab number of images in swapchain
		const uint32_t swapchainImageCount = m_swapchainImages.size();
		m_framebuffers.resize(swapchainImageCount);

		for (int i = 0; i < swapchainImageCount; i++)
		{
			std::array<vk::ImageView, 2> attachments = { m_swapchainImageViews[i], m_swapchainDepthImage.imageView };

			fbInfo.pAttachments = attachments.data();
			fbInfo.attachmentCount = attachments.size();

			VK_CHECK(m_device.createFramebuffer(&fbInfo, nullptr, &m_framebuffers[i]));
		}

		m_deletionQueue.PushFunction([=]()
		{
			for (int i = 0; i < m_framebuffers.size(); i++)
			{
				m_device.destroyFramebuffer(m_framebuffers[i]);
			}
		});
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

			// Material Buffers

			// Object Buffers

			m_deletionQueue.PushFunction([=]()
			{
				m_allocator.destroyBuffer(m_frameRenderData[i].objectBuffer.buffer, m_frameRenderData[i].objectBuffer.allocation);
				m_allocator.destroyBuffer(m_frameRenderData[i].cameraBuffer.buffer, m_frameRenderData[i].cameraBuffer.allocation);
			});
		}
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

			Util::DescriptorBuilder::Begin(m_staticRenderData.descriptorLayoutCache, m_staticRenderData.descriptorAllocator)
				.BindBuffer(0, &cameraBufferInfo, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex )
				.BindBuffer(1, &objectBufferInfo, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eVertex)
				.Build(m_frameRenderData[i].globalDescriptor, m_staticRenderData.globalSetLayout);

			// Material Descriptors



			// Instance Descriptors



		}

		m_deletionQueue.PushFunction([=]()
		{
			m_staticRenderData.descriptorLayoutCache = nullptr;
			m_staticRenderData.descriptorAllocator = nullptr;
		});
	}

	void VKRenderSystem::InitPipelines()
	{
		BuildTrianglePipeline();
		BuildForwardRendererPipeline();
	}

	void VKRenderSystem::BuildTrianglePipeline()
	{
		m_triVertMod = vku::ShaderModule { m_device, "C:\\Projects\\PuffinEngine\\bin\\vulkan\\triangle\\triangle_vs.spv" };
		m_triFragMod = vku::ShaderModule { m_device, "C:\\Projects\\PuffinEngine\\bin\\vulkan\\triangle\\triangle_fs.spv" };

		vku::PipelineLayoutMaker plm{};
		m_triPipelineLayout = plm.createUnique(m_device);

		vku::PipelineMaker pm{ m_windowSize.width, m_windowSize.height };
		m_triPipeline = pm
			.shader(vk::ShaderStageFlagBits::eVertex, m_triVertMod)
			.shader(vk::ShaderStageFlagBits::eFragment, m_triFragMod)
			.createUnique(m_device, m_pipelineCache, *m_triPipelineLayout, m_renderPass);

		m_device.destroyShaderModule(m_triVertMod.module());
		m_device.destroyShaderModule(m_triFragMod.module());

		m_deletionQueue.PushFunction([=]()
		{
			m_triPipelineLayout = {};
			m_triPipeline = {};
		});
	}

	void VKRenderSystem::BuildForwardRendererPipeline()
	{
		m_forwardVertMod = vku::ShaderModule{ m_device, "C:\\Projects\\PuffinEngine\\bin\\vulkan\\forward_shading\\forward_shading_vs.spv" };
		m_forwardFragMod = vku::ShaderModule{ m_device, "C:\\Projects\\PuffinEngine\\bin\\vulkan\\forward_shading\\forward_shading_fs.spv" };

		vku::PipelineLayoutMaker plm{};
		m_forwardPipelineLayout = plm
			.descriptorSetLayout(m_staticRenderData.globalSetLayout)
			.createUnique(m_device);

		vk::PipelineDepthStencilStateCreateInfo depthStencilInfo = { {}, true, true,
			vk::CompareOp::eLessOrEqual, false, false, {}, {}, 0.0f, 1.0f };

		vku::PipelineMaker pm{ m_windowSize.width, m_windowSize.height };
		m_forwardPipeline = pm
			.shader(vk::ShaderStageFlagBits::eVertex, m_forwardVertMod)
			.shader(vk::ShaderStageFlagBits::eFragment, m_forwardFragMod)
			.depthStencilState(depthStencilInfo)
			.vertexBinding(0, sizeof(VertexPNTV32))
			.vertexAttribute(0, 0, vk::Format::eR32G32B32Sfloat, offsetof(VertexPNTV32, pos))
			.vertexAttribute(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(VertexPNTV32, normal))
			.vertexAttribute(2, 0, vk::Format::eR32G32B32Sfloat, offsetof(VertexPNTV32, tangent))
			.vertexAttribute(3, 0, vk::Format::eR32G32Sfloat, offsetof(VertexPNTV32, uv))
			.createUnique(m_device, m_pipelineCache, *m_forwardPipelineLayout, m_renderPass);

		m_device.destroyShaderModule(m_forwardVertMod.module());
		m_device.destroyShaderModule(m_forwardFragMod.module());

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
		}

		std::vector<std::shared_ptr<ECS::Entity>> camEntities;
		ECS::GetEntities<TransformComponent, CameraComponent, CameraMatComponent>(m_world, camEntities);
		for (const auto& entity : camEntities)
		{
			UpdateCameraComponent(entity);
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
	}

	void VKRenderSystem::Draw()
	{
		// Wait until GPU has finished rendering last frame. Timeout of 1 second
		VK_CHECK(m_device.waitForFences(1, &GetCurrentFrameData().renderFence, true, 1000000000));
		VK_CHECK(m_device.resetFences(1, &GetCurrentFrameData().renderFence));

		uint32_t swapchainImageIdx;
		VK_CHECK(m_device.acquireNextImageKHR(m_swapchain, 1000000000, GetCurrentFrameData().presentSemaphore, nullptr, &swapchainImageIdx));

		vk::CommandBuffer cmd = GetCurrentFrameData().mainCommandBuffer;

		// Reset command buffer for recording new commands
		cmd.reset();

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
		vk::RenderPassBeginInfo rpInfo = { m_renderPass, m_framebuffers[swapchainImageIdx],
			vk::Rect2D{ {0, 0}, m_windowSize }, clearValues.size(), clearValues.data(), nullptr };

		cmd.beginRenderPass(&rpInfo, vk::SubpassContents::eInline);

		//cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_triPipeline.get());
		//cmd.draw(3, 1, 0, 0);

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
			1, &GetCurrentFrameData().renderSemaphore, 1, &m_swapchain, &swapchainImageIdx
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

	void VKRenderSystem::PrepareSceneData()
	{
		// Prepare Camera Data

		/*Vector3f camPos = { 0.0f, 0.0f, -10.0f };

		float aspect = (float)m_windowSize.width / (float)m_windowSize.height;

		glm::mat4 view = glm::translate(glm::mat4(1.f), static_cast<glm::vec3>(camPos));
		glm::mat4 projection = glm::perspective(glm::radians(70.0f), aspect, 0.1f, 200.0f);
		projection[1][1] *= -1;*/

		GPUCameraData camData;
		camData.proj = m_editorCamMats.proj;
		camData.view = m_editorCamMats.view;
		camData.viewProj = m_editorCamMats.viewProj;

		void* data;
		VK_CHECK(m_allocator.mapMemory(GetCurrentFrameData().cameraBuffer.allocation, &data));

		memcpy(data, &camData, sizeof(GPUCameraData));

		m_allocator.unmapMemory(GetCurrentFrameData().cameraBuffer.allocation);

		// Prepare Object Data

		void* objectData;
		VK_CHECK(m_allocator.mapMemory(GetCurrentFrameData().objectBuffer.allocation, &objectData));

		GPUObjectData* objectSSBO = (GPUObjectData*)objectData;

		int i = 0;

		for (const auto [fst, snd] : m_meshDrawList)
		{
			for (const auto entityID : snd)
			{
				const auto& transform = m_world->GetComponent<TransformComponent>(entityID);

				objectSSBO[i].model = BuildModelTransform(transform.position, transform.rotation, transform.scale);

				i++;
			}
		}

		m_allocator.unmapMemory(GetCurrentFrameData().objectBuffer.allocation);
	}

	void VKRenderSystem::DrawObjects(vk::CommandBuffer cmd)
	{
		cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_forwardPipeline.get());

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
}
