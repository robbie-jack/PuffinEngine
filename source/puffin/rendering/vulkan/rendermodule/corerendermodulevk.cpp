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
		RenderModuleVK::RegisterModules();
	}

	void CoreRenderModuleVK::Initialize()
	{
		RenderModuleVK::Initialize();
		
		InitBuffers();
		InitSamplers();
		InitDescriptorLayouts();
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

		mRenderSubsystem->GetResourceManager()->DestroyResource(mLightDescriptorLayoutID);
		mLightDescriptorLayoutID = gInvalidID;

		mRenderSubsystem->GetResourceManager()->DestroyResource(mMatDescriptorLayoutID);
		mMatDescriptorLayoutID = gInvalidID;

		RenderModuleVK::Deinitialize();
	}

	void CoreRenderModuleVK::PostInitialize()
	{
		RenderModuleVK::PostInitialize();

		InitDescriptorLayouts();
	}

	void CoreRenderModuleVK::UpdateResources(ResourceManagerVK* resourceManager)
	{
		RenderModuleVK::UpdateResources(resourceManager);
	}

	void CoreRenderModuleVK::UpdateGraph(RenderGraphVK& renderGraph)
	{
		RenderModuleVK::UpdateGraph(renderGraph);
	}

	void CoreRenderModuleVK::PreRender(double deltaTime)
	{
		RenderModuleVK::PreRender(deltaTime);
		
		UpdateRenderData();
		ProcessComponents();
		UpdateTextureDescriptors();
		PrepareSceneData();
		BuildIndirectCommands();
	}

	void CoreRenderModuleVK::OnUpdateMesh(entt::registry& registry, entt::entity entity)
	{

	}

	void CoreRenderModuleVK::OnUpdateTransform(entt::registry& registry, entt::entity entity)
	{

	}

	void CoreRenderModuleVK::OnDestroyMeshOrTransform(entt::registry& registry, entt::entity entity)
	{

	}

	void CoreRenderModuleVK::AddRenderable(entt::registry& registry, entt::entity entity)
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

	void CoreRenderModuleVK::InitDescriptorLayouts()
	{
		ResourceManagerVK* resourceManager = mRenderSubsystem->GetResourceManager();
		
		DescriptorLayoutDescVK descriptorLayoutDesc;
		
		// Object Layout
		{
			descriptorLayoutDesc.bindings.push_back({ vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eVertex });
			
			mObjectDescriptorLayoutID = resourceManager->CreateOrUpdateDescriptorLayout(descriptorLayoutDesc, "objects");
		}

		// Light Layout
		{
			descriptorLayoutDesc.bindings.clear();

			descriptorLayoutDesc.bindings.push_back({ vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eFragment });
			descriptorLayoutDesc.bindings.push_back({ vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eFragment });
			descriptorLayoutDesc.bindings.push_back({ vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eFragment });

			mLightDescriptorLayoutID = resourceManager->CreateOrUpdateDescriptorLayout(descriptorLayoutDesc, "lights");
		}

		// Material Layout
		{
			descriptorLayoutDesc.bindings.clear();

			descriptorLayoutDesc.bindings.push_back({ vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eFragment });
			descriptorLayoutDesc.bindings.push_back({ vk::DescriptorType::eCombinedImageSampler, gMaxMaterialInstances, vk::ShaderStageFlagBits::eFragment, 
				{ vk::DescriptorBindingFlagBits::ePartiallyBound | vk::DescriptorBindingFlagBits::eVariableDescriptorCount } });

			mMatDescriptorLayoutID = resourceManager->CreateOrUpdateDescriptorLayout(descriptorLayoutDesc, "materials");
		}

		// Shadow Layout - PUFFIN_TODO - Dummy layout, will be removed once shadows are implemented in own render module
		{
			constexpr uint32_t imageCount = 128;
			
			descriptorLayoutDesc.bindings.clear();

			descriptorLayoutDesc.bindings.push_back({ vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eFragment });
			descriptorLayoutDesc.bindings.push_back({ vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eFragment });
			descriptorLayoutDesc.bindings.push_back({ vk::DescriptorType::eCombinedImageSampler, imageCount, vk::ShaderStageFlagBits::eFragment,
				{ vk::DescriptorBindingFlagBits::ePartiallyBound | vk::DescriptorBindingFlagBits::eVariableDescriptorCount } });

			mShadowDescriptorLayoutID = resourceManager->CreateOrUpdateDescriptorLayout(descriptorLayoutDesc, "shadows");
		}
	}

	void CoreRenderModuleVK::InitDescriptorSets()
	{
		ResourceManagerVK* resourceManager = mRenderSubsystem->GetResourceManager();
		util::DescriptorAllocator* descriptorAllocator = mRenderSubsystem->GetDescriptorAllocator();
		util::DescriptorLayoutCache* descriptorLayoutCache = mRenderSubsystem->GetDescriptorLayoutCache();
		
		DescriptorLayoutDescVK descriptorLayoutDesc;

		for (int i = 0; i < gBufferedFrameCount; ++i)
		{
			// Object Descriptor Set
			{
				vk::DescriptorBufferInfo objectBufferInfo =
				{
					resourceManager->GetBuffer(mObjectBufferID, i).buffer, 0, sizeof(GPUObjectData) * gMaxObjects,
				};

				util::DescriptorBuilder::Begin(descriptorLayoutCache, descriptorAllocator)
				.BindBuffer(0, &objectBufferInfo, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eVertex)
				.Build(mFrameRenderData[i].objectDescriptor, resourceManager->GetDescriptorLayout(mObjectDescriptorLayoutID));
			}

			// Light Descriptor Set
			{
				vk::DescriptorBufferInfo pointLightBufferInfo =
				{
					resourceManager->GetBuffer(mPointLightBufferID, i).buffer, 0, sizeof(GPUPointLightData) * gMaxPointLights,
				};

				vk::DescriptorBufferInfo spotLightBufferInfo =
				{
					resourceManager->GetBuffer(mSpotLightBufferID, i).buffer, 0, sizeof(GPUSpotLightData) * gMaxSpotLights,
				};

				vk::DescriptorBufferInfo dirLightBufferInfo =
				{
					resourceManager->GetBuffer(mDirLightBufferID, i).buffer, 0, sizeof(GPUDirLightData) * gMaxDirectionalLights,
				};

				util::DescriptorBuilder::Begin(descriptorLayoutCache, descriptorAllocator)
				.BindBuffer(0, &pointLightBufferInfo, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eFragment)
				.BindBuffer(1, &spotLightBufferInfo, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eFragment)
				.BindBuffer(2, &dirLightBufferInfo, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eFragment)
				.Build(mFrameRenderData[i].lightDescriptor, resourceManager->GetDescriptorLayout(mLightDescriptorLayoutID));
			}
			
			// Material Descriptor Set
			{
				constexpr uint32_t imageCount = 128;
				
				constexpr vk::DescriptorBindingFlags descriptorBindingFlags = { vk::DescriptorBindingFlagBits::ePartiallyBound | vk::DescriptorBindingFlagBits::eVariableDescriptorCount };
				vk::DescriptorSetVariableDescriptorCountAllocateInfo variableDescriptorCountAllocInfo =
				{ 1, &imageCount };

				vk::DescriptorBufferInfo materialBufferInfo = {
					resourceManager->GetBuffer(mMaterialInstanceBufferID, i).buffer, 0, sizeof(GPUMaterialInstanceData) * gMaxMaterialInstances
				};

				util::DescriptorBuilder::Begin(descriptorLayoutCache, descriptorAllocator)
				.BindBuffer(0, &materialBufferInfo, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eFragment)
				.BindImagesWithoutWrite(1, imageCount, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment, descriptorBindingFlags)
				.AddPNext(&variableDescriptorCountAllocInfo)
				.Build(mFrameRenderData[i].materialDescriptor, resourceManager->GetDescriptorLayout(mMatDescriptorLayoutID));
			}

			// Shadow Descriptor Set - PUFFIN_TODO - Dummy set, will be removed once shadows are implemented in own render module
			{
				constexpr uint32_t lightCount = gMaxLights;
			}
		}
	}

	void CoreRenderModuleVK::UpdateRenderData()
	{

	}

	void CoreRenderModuleVK::ProcessComponents()
	{

	}

	void CoreRenderModuleVK::UpdateTextureDescriptors()
	{

	}

	void CoreRenderModuleVK::PrepareSceneData()
	{

	}

	void CoreRenderModuleVK::BuildIndirectCommands()
	{

	}
}
