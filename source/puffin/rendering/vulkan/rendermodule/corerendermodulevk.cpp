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
		InitSamplers();
		InitDescriptors();
	}

	void CoreRenderModuleVK::Deinitialize()
	{
		mRenderSubsystem->GetResourceManager()->DestroyResource(mIndirectDrawBufferID);
		mIndirectDrawBufferID = gInvalidID;

		mRenderSubsystem->GetResourceManager()->DestroyResource(mCameraBufferID);
		mCameraBufferID = gInvalidID;

		mRenderSubsystem->GetResourceManager()->DestroyResource(mPointLightBufferID);
		mPointLightBufferID = gInvalidID;

		mRenderSubsystem->GetResourceManager()->DestroyResource(mSpotLightBufferID);
		mSpotLightBufferID = gInvalidID;

		mRenderSubsystem->GetResourceManager()->DestroyResource(mDirLightBufferID);
		mDirLightBufferID = gInvalidID;

		mRenderSubsystem->GetResourceManager()->DestroyResource(mObjectBufferID);
		mObjectBufferID = gInvalidID;

		mRenderSubsystem->GetResourceManager()->DestroyResource(mMaterialInstanceBufferID);
		mMaterialInstanceBufferID = gInvalidID;

		mRenderSubsystem->GetDevice().destroySampler(mTextureSampler);

		mRenderSubsystem->GetResourceManager()->DestroyResource(mObjectDescriptorLayoutID);
		mObjectDescriptorLayoutID = gInvalidID;

		mRenderSubsystem->GetResourceManager()->DestroyResource(mGlobalDescriptorLayoutID);
		mGlobalDescriptorLayoutID = gInvalidID;

		mRenderSubsystem->GetResourceManager()->DestroyResource(mTextureDescriptorLayoutID);
		mTextureDescriptorLayoutID = gInvalidID;
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
		mIndirectDrawBufferID = resourceManager->CreateOrUpdateBuffer(bufferDesc, "indirect_draw");

		// Camera Buffer
		bufferDesc.size = sizeof(GPUCameraData);
		bufferDesc.usage = vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferDst;
		mCameraBufferID = resourceManager->CreateOrUpdateBuffer(bufferDesc, "camera");

		// Light Buffers
		bufferDesc.size = sizeof(GPUPointLightData) * gMaxPointLights;
		bufferDesc.usage = vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst;
		mPointLightBufferID = resourceManager->CreateOrUpdateBuffer(bufferDesc, "point_lights");

		bufferDesc.size = sizeof(GPUSpotLightData) * gMaxSpotLights;
		mSpotLightBufferID = resourceManager->CreateOrUpdateBuffer(bufferDesc, "spot_lights");

		bufferDesc.size = sizeof(GPUDirLightData) * gMaxDirectionalLights;
		mDirLightBufferID = resourceManager->CreateOrUpdateBuffer(bufferDesc, "dir_lights");

		// Object Buffer

		bufferDesc.size = sizeof(GPUObjectData) * gMaxObjects;
		mObjectBufferID = resourceManager->CreateOrUpdateBuffer(bufferDesc, "objects");

		// Material Buffer

		bufferDesc.size = sizeof(GPUMaterialInstanceData) * gMaxMaterialInstances;
		mMaterialInstanceBufferID = resourceManager->CreateOrUpdateBuffer(bufferDesc, "material_instances");

	}

	void CoreRenderModuleVK::InitSamplers()
	{
		vk::SamplerCreateInfo textureSamplerInfo = {};
		textureSamplerInfo.anisotropyEnable = true;
		textureSamplerInfo.maxAnisotropy = 16.0f;

		mTextureSampler = mRenderSubsystem->GetDevice().createSampler(textureSamplerInfo);
	}

	void CoreRenderModuleVK::InitDescriptors()
	{
		ResourceManagerVK* resourceManager = mRenderSubsystem->GetResourceManager();

		// Object Layout
		DescriptorLayoutDescVK descriptorLayoutDesc;

		descriptorLayoutDesc.bindings.push_back({ vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eVertex });

		mObjectDescriptorLayoutID = resourceManager->CreateOrUpdateDescriptorLayout(descriptorLayoutDesc, "objects");

		// Global Layout
		descriptorLayoutDesc.bindings.clear();

		descriptorLayoutDesc.bindings.push_back({ vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex });
		descriptorLayoutDesc.bindings.push_back({ vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eFragment });
		descriptorLayoutDesc.bindings.push_back({ vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eFragment });
		descriptorLayoutDesc.bindings.push_back({ vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eFragment });
		descriptorLayoutDesc.bindings.push_back({ vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eFragment });
		descriptorLayoutDesc.bindings.push_back({ vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eFragment });
		descriptorLayoutDesc.bindings.push_back({ vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eFragment });

		mGlobalDescriptorLayoutID = resourceManager->CreateOrUpdateDescriptorLayout(descriptorLayoutDesc, "global");

		// Texture Layout
		descriptorLayoutDesc.bindings.clear();

		constexpr uint32_t imageCount = 128;

		descriptorLayoutDesc.bindings.push_back({ vk::DescriptorType::eCombinedImageSampler, imageCount, vk::ShaderStageFlagBits::eFragment, 
			{ vk::DescriptorBindingFlagBits::ePartiallyBound | vk::DescriptorBindingFlagBits::eVariableDescriptorCount } });

		mTextureDescriptorLayoutID = resourceManager->CreateOrUpdateDescriptorLayout(descriptorLayoutDesc, "textures");
	}
}
