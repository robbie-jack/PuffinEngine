#include "puffin/rendering/vulkan/rendermodule/corerendermodulevk.h"

#include "vk_mem_alloc.hpp"

#include "puffin/rendering/renderglobals.h"
#include "puffin/rendering/vulkan/rendersubsystemvk.h"
#include "puffin/rendering/vulkan/resourcedescvk.h"
#include "puffin/rendering/vulkan/resourcemanagervk.h"

namespace puffin::rendering
{
	CoreRenderModuleVK::CoreRenderModuleVK(std::shared_ptr<core::Engine> engine, RenderSubsystemVK* renderSubsystem)
		: RenderModuleVK(engine, renderSubsystem)
	{
	}

	void CoreRenderModuleVK::RegisterModules()
	{
		
	}

	void CoreRenderModuleVK::Initialize()
	{
		InitBuffers();
	}

	void CoreRenderModuleVK::Deinitialize()
	{
		mRenderSubsystem->GetResourceManager()->DestroyResource(mIndirectDrawBufferID);
		mIndirectDrawBufferID = gInvalidID;
	}

	void CoreRenderModuleVK::UpdateResources(ResourceManagerVK* resourceManager)
	{
		
	}

	void CoreRenderModuleVK::UpdateGraph(RenderGraphVK& renderGraph)
	{
		
	}

	void CoreRenderModuleVK::PreRender(double deltaTime)
	{
		
	}

	void CoreRenderModuleVK::InitBuffers()
	{
		ResourceManagerVK* resourceManager = mRenderSubsystem->GetResourceManager();

		// Indirect Buffer
		BufferDescVK bufferDesc;
		bufferDesc.size = sizeof(vk::DrawIndexedIndirectCommand) * gMaxObjects;
		bufferDesc.usage = vk::BufferUsageFlagBits::eIndirectBuffer | vk::BufferUsageFlagBits::eStorageBuffer |
			vk::BufferUsageFlagBits::eTransferDst;
		bufferDesc.allocFlags = vma::AllocationCreateFlagBits::eHostAccessSequentialWrite | vma::AllocationCreateFlagBits::eMapped;
		mIndirectDrawBufferID = resourceManager->CreateOrUpdateBuffer(bufferDesc);

		// Camera Buffer
		bufferDesc.size = sizeof(GPUCameraData);
		bufferDesc.usage = vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferDst;
		mCameraBufferID = resourceManager->CreateOrUpdateBuffer(bufferDesc);

		// Light Buffers
		bufferDesc.size = sizeof(GPUPointLightData) * gMaxPointLights;
		bufferDesc.usage = vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst;
		mPointLightBufferID = resourceManager->CreateOrUpdateBuffer(bufferDesc);

		bufferDesc.size = sizeof(GPUSpotLightData) * gMaxSpotLights;
		mSpotLightBufferID = resourceManager->CreateOrUpdateBuffer(bufferDesc);

		bufferDesc.size = sizeof(GPUDirLightData) * gMaxDirectionalLights;
		mDirLightBufferID = resourceManager->CreateOrUpdateBuffer(bufferDesc);

		// Object Buffer

		bufferDesc.size = sizeof(GPUObjectData) * gMaxObjects;
		mObjectBufferID = resourceManager->CreateOrUpdateBuffer(bufferDesc);

		// Material Buffer

		bufferDesc.size = sizeof(GPUMaterialInstanceData) * gMaxMaterialInstances;
		mMaterialInstanceBufferID = resourceManager->CreateOrUpdateBuffer(bufferDesc);

	}
}
