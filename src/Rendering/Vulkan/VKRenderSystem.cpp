#include "Rendering/Vulkan/VKRenderSystem.hpp"

#define VMA_IMPLEMENTATION

// If you don't like the `vma::` prefix:
//#define VMA_HPP_NAMESPACE <prefix>

#include <iostream>

#include "vk_mem_alloc.h"
#include "vk_mem_alloc.hpp"
#include "VkBootstrap.h"

#include "Window/WindowSubsystem.hpp"
#include "Engine/Engine.hpp"
#include "Rendering/Vulkan/VKHelpers.hpp"
#include "Components/Rendering/MeshComponent.h"
#include "Assets/AssetRegistry.h"
#include "Assets/MeshAsset.h"

#include "Components/TransformComponent.h"

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
		InitPipelines();

		InitComponents();

		m_isInitialized = true;
	}

	void VKRenderSystem::Update()
	{
		UpdateComponents();

		std::vector<UUID> removedMeshIDs;

		for (const auto& meshData : m_meshData)
		{
			if (meshData.entities.empty())
			{
				m_allocator.destroyBuffer(meshData.vertexBuffer.buffer, meshData.vertexBuffer.allocation);
				m_allocator.destroyBuffer(meshData.indexBuffer.buffer, meshData.indexBuffer.allocation);

				removedMeshIDs.push_back(meshData.assetID);
			}
		}

		for (const auto& meshID : removedMeshIDs)
		{
			m_meshData.Erase(meshID);
		}

		Draw();
	}

	void VKRenderSystem::Cleanup()
	{
		m_device.waitIdle();

		if (m_isInitialized)
		{
			CleanupComponents();

			m_deletionQueue.Flush();

			m_isInitialized = false;
		}
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
		vkb::Device vkbDevice = deviceBuilder.build().value();

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

		m_deletionQueue.PushFunction([=]()
		{
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
		VK_CHECK(m_device.createCommandPool(&commandPoolInfo, nullptr, &m_commandPool));

		vk::CommandBufferAllocateInfo commandBufferInfo = { m_commandPool, vk::CommandBufferLevel::ePrimary, 1 };
		VK_CHECK(m_device.allocateCommandBuffers(&commandBufferInfo, &m_mainCommandBuffer));

		// Init Upload Context Command Pool/Buffer
		commandPoolInfo = { vk::CommandPoolCreateFlagBits::eResetCommandBuffer, m_graphicsQueueFamily };
		VK_CHECK(m_device.createCommandPool(&commandPoolInfo, nullptr, &m_uploadContext.commandPool));

		commandBufferInfo = { m_commandPool, vk::CommandBufferLevel::ePrimary, 1 };
		VK_CHECK(m_device.allocateCommandBuffers(&commandBufferInfo, &m_uploadContext.commandBuffer));

		m_deletionQueue.PushFunction([=]()
		{
			m_device.destroyCommandPool(m_commandPool);
			m_device.destroyCommandPool(m_uploadContext.commandPool);
		});
	}

	void VKRenderSystem::InitDefaultRenderPass()
	{
		vk::AttachmentDescription colorAttachment = 
		{
			{}, m_swapchainImageFormat, vk::SampleCountFlagBits::e1,
			vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
			vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
			vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR
		};

		vk::AttachmentReference colorAttachmentRef = { 0, vk::ImageLayout::eColorAttachmentOptimal };

		vk::SubpassDescription subpass = { {}, vk::PipelineBindPoint::eGraphics, 0, nullptr, 1, &colorAttachmentRef };

		vk::RenderPassCreateInfo renderPassInfo = { {}, 1, &colorAttachment, 1, &subpass };

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
			fbInfo.pAttachments = &m_swapchainImageViews[i];
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

		VK_CHECK(m_device.createFence(&fenceCreateInfo, nullptr, &m_renderFence));

		// Init Upload Context Fence
		fenceCreateInfo = vk::FenceCreateInfo{ {}, nullptr };
		VK_CHECK(m_device.createFence(&fenceCreateInfo, nullptr, &m_uploadContext.uploadFence));

		vk::SemaphoreCreateInfo semaphoreCreateInfo = { {}, nullptr };

		VK_CHECK(m_device.createSemaphore(&semaphoreCreateInfo, nullptr, &m_presentSemaphore));
		VK_CHECK(m_device.createSemaphore(&semaphoreCreateInfo, nullptr, &m_renderSemaphore));

		m_deletionQueue.PushFunction([=]()
		{
			m_device.destroyFence(m_renderFence);
			m_device.destroyFence(m_uploadContext.uploadFence);

			m_device.destroySemaphore(m_presentSemaphore);
			m_device.destroySemaphore(m_renderSemaphore);
		});
	}

	void VKRenderSystem::InitPipelines()
	{
		BuildTrianglePipeline();		
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
			/*.vertexBinding(0, sizeof(VertexPC32))
			.vertexAttribute(0, 0, vk::Format::eR32G32Sfloat, offsetof(VertexPC32, pos))
			.vertexAttribute(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(VertexPC32, color))*/
			.createUnique(m_device, m_pipelineCache, *m_triPipelineLayout, m_renderPass);

		m_device.destroyShaderModule(m_triVertMod.module());
		m_device.destroyShaderModule(m_triFragMod.module());

		m_deletionQueue.PushFunction([=]()
		{
			m_triPipelineLayout = {};
			m_triPipeline = {};
		});
	}

	void VKRenderSystem::InitComponents()
	{
		std::vector<std::shared_ptr<ECS::Entity>> meshEntities;
		ECS::GetEntities<TransformComponent, MeshComponent>(m_world, meshEntities);
		for (const auto& entity : meshEntities)
		{
			if (entity->GetComponentFlag<MeshComponent, FlagDirty>())
			{
				InitMeshComponent(entity);

				entity->SetComponentFlag<MeshComponent, FlagDirty>(false);
			}
		}
	}

	void VKRenderSystem::UpdateComponents()
	{
		std::vector<std::shared_ptr<ECS::Entity>> meshEntities;
		ECS::GetEntities<TransformComponent, MeshComponent>(m_world, meshEntities);
		for (const auto& entity : meshEntities)
		{
			if (entity->GetComponentFlag<MeshComponent, FlagDirty>())
			{
				CleanupMeshComponent(entity);
				InitMeshComponent(entity);

				entity->SetComponentFlag<MeshComponent, FlagDirty>(false);
			}
		}
	}

	void VKRenderSystem::CleanupComponents()
	{
		std::vector<std::shared_ptr<ECS::Entity>> meshEntities;
		ECS::GetEntities<TransformComponent, MeshComponent>(m_world, meshEntities);
		for (const auto& entity : meshEntities)
		{
			CleanupMeshComponent(entity);
		}
	}

	void VKRenderSystem::Draw()
	{
		// Wait until GPU has finished rendering last frame. Timeout of 1 second
		VK_CHECK(m_device.waitForFences(1, &m_renderFence, true, 1000000000));
		VK_CHECK(m_device.resetFences(1, &m_renderFence));

		uint32_t swapchainImageIdx;
		VK_CHECK(m_device.acquireNextImageKHR(m_swapchain, 1000000000, m_presentSemaphore, nullptr, &swapchainImageIdx));

		vk::CommandBuffer cmd = m_mainCommandBuffer;

		// Reset command buffer for recording new commands
		cmd.reset();

		// Begin command buffer execution
		vk::CommandBufferBeginInfo cmdBeginInfo = { vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
			nullptr, nullptr };

		VK_CHECK(cmd.begin(&cmdBeginInfo));

		vk::ClearValue clearValue;
		float flash = abs(sin(m_frameNumber / 120.0f));
		clearValue.color = { 0.0f, 0.0f, flash, 1.0f };

		// Begin main renderpass
		vk::RenderPassBeginInfo rpInfo = { m_renderPass, m_framebuffers[swapchainImageIdx],
			vk::Rect2D{ {0, 0}, m_windowSize }, 1, &clearValue, nullptr };

		cmd.beginRenderPass(&rpInfo, vk::SubpassContents::eInline);

		cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_triPipeline.get());
		cmd.draw(3, 1, 0, 0);

		// End main renderpass
		cmd.endRenderPass();

		// Finish command buffer recording
		cmd.end();

		// Prepare submission to queue
		vk::PipelineStageFlags waitStage = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
		vk::SubmitInfo submit = 
		{
			1, &m_presentSemaphore,
			&waitStage, 1, &cmd,
			1, &m_renderSemaphore, nullptr
		};

		VK_CHECK(m_graphicsQueue.submit(1, &submit, m_renderFence));

		vk::PresentInfoKHR presentInfo =
		{
			1, &m_renderSemaphore, 1, &m_swapchain, &swapchainImageIdx
		};

		VK_CHECK(m_graphicsQueue.presentKHR(&presentInfo));

		m_frameNumber++;
	}

	void VKRenderSystem::InitMeshComponent(std::shared_ptr<ECS::Entity> entity)
	{
		auto& mesh = entity->GetComponent<MeshComponent>();

		if (!m_meshData.Contains(mesh.meshAssetID))
		{
			const auto meshAsset = std::static_pointer_cast<Assets::StaticMeshAsset>(Assets::AssetRegistry::Get()->GetAsset(mesh.meshAssetID));

			if (meshAsset && meshAsset->Load())
			{
				MeshData meshData;
				meshData.assetID = mesh.meshAssetID;

				meshData.vertexBuffer = Util::InitVertexBuffer(shared_from_this(), meshAsset->GetVertices().data(), 
					meshAsset->GetNumVertices(), meshAsset->GetVertexSize());

				meshData.indexBuffer = Util::InitIndexBuffer(shared_from_this(), meshAsset->GetIndices().data(),
					meshAsset->GetNumIndices(), meshAsset->GetIndexSize());

				m_meshData.Insert(mesh.meshAssetID, meshData);

				meshAsset->Unload();
			}
		}

		m_meshData[mesh.meshAssetID].entities.insert(entity->ID());
	}

	void VKRenderSystem::CleanupMeshComponent(std::shared_ptr<ECS::Entity> entity)
	{
		auto& mesh = entity->GetComponent<MeshComponent>();

		if (m_meshData.Contains(mesh.meshAssetID))
		{
			m_meshData[mesh.meshAssetID].entities.erase(entity->ID());

			if (m_meshData[mesh.meshAssetID].entities.empty())
			{
				m_allocator.destroyBuffer(m_meshData[mesh.meshAssetID].vertexBuffer.buffer, m_meshData[mesh.meshAssetID].vertexBuffer.allocation);
				m_allocator.destroyBuffer(m_meshData[mesh.meshAssetID].indexBuffer.buffer, m_meshData[mesh.meshAssetID].indexBuffer.allocation);

				m_meshData.Erase(mesh.meshAssetID);
			}
		}
	}
}