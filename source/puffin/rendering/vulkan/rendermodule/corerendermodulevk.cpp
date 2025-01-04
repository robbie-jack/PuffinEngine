#include "puffin/rendering/vulkan/rendermodule/corerendermodulevk.h"

#include <glm/gtx/rotate_vector.hpp>

#include "vk_mem_alloc.hpp"
#include "puffin/assets/assetregistry.h"
#include "puffin/assets/staticmeshasset.h"

#include "puffin/rendering/renderglobals.h"
#include "puffin/rendering/vulkan/rendersubsystemvk.h"
#include "puffin/rendering/vulkan/resourcedescvk.h"
#include "puffin/rendering/vulkan/resourcemanagervk.h"
#include "puffin/components/transformcomponent2d.h"
#include "puffin/components/transformcomponent3d.h"
#include "puffin/components/physics/3d/velocitycomponent3d.h"
#include "puffin/components/rendering/3d/directionallightcomponent3d.h"
#include "puffin/components/rendering/3d/pointlightcomponent3d.h"
#include "puffin/components/rendering/3d/shadowcastercomponent3d.h"
#include "puffin/components/rendering/3d/spotlightcomponent3d.h"
#include "puffin/components/rendering/3d/staticmeshcomponent3d.h"
#include "puffin/core/enkitssubsystem.h"
#include "puffin/nodes/transformnode3d.h"
#include "puffin/rendering/camerasubsystem.h"
#include "puffin/rendering/vulkan/helpersvk.h"
#include "puffin/rendering/vulkan/unifiedgeometrybuffervk.h"
#include "puffin/rendering/vulkan/materialregistryvk.h"
#include "puffin/rendering/vulkan/texturemanagervk.h"
#include "puffin/scene/scenegraphsubsystem.h"

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

		BindCallbacks();
		InitBuffers();
		InitSamplers();
		InitDescriptorLayouts();

		mUpdateRenderables = true;
		mInitialized = true;
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

		mInitialized = false;

		RenderModuleVK::Deinitialize();
	}

	void CoreRenderModuleVK::PostInitialize()
	{
		RenderModuleVK::PostInitialize();

		InitDescriptorSets();
	}

	void CoreRenderModuleVK::UpdateResources(ResourceManagerVK* resourceManager)
	{
		RenderModuleVK::UpdateResources(resourceManager);

		// Load Meshes
		{
			for (const auto meshID : mMeshesToLoad)
			{
				if (const auto staticMesh = assets::AssetRegistry::Get()->GetAsset<assets::StaticMeshAsset>(meshID))
				{
					mRenderSubsystem->GetUnifiedGeometryBuffer()->AddStaticMesh(staticMesh);
				}
			}

			mMeshesToLoad.clear();
		}
	}

	void CoreRenderModuleVK::UpdateGraph(RenderGraphVK& renderGraph)
	{
		RenderModuleVK::UpdateGraph(renderGraph);
	}

	void CoreRenderModuleVK::PreRender(double deltaTime)
	{
		RenderModuleVK::PreRender(deltaTime);

		// Upload material data to gpu when updated
		{
			if (mRenderSubsystem->GetMaterialRegistry()->MaterialDataNeedsUploaded())
			{
				for (uint32_t i = 0; i < gBufferedFrameCount; i++)
				{
					mFrameRenderData[i].updateGPUMaterialData = true;
				}
			}
		}

		// Update texture data and descriptor if needed
		{
			const auto& textureManager = mRenderSubsystem->GetTextureManager();
			
			if (textureManager->TextureDescriptorNeedsUpdated() == true)
			{
				mTexData.Clear();

				for (const auto& [id, texture] : textureManager->GetLoadedTextures())
				{
					TextureDataVK texData;
					texData.id = id;
					texData.sampler = mTextureSampler;
					
					mTexData.Emplace(id, texData);
				}
				
				for (int i = 0; i < gBufferedFrameCount; i++)
				{
					mFrameRenderData[i].textureDescriptorNeedsUpdated = true;
				}
			}
		}
		
		ProcessComponents();
		UpdateTextureDescriptors();
		PrepareSceneData();
		BuildIndirectCommands();
	}

	void CoreRenderModuleVK::RecordCopyCommand(vk::CommandBuffer& cmd, const AllocatedImage& imageToCopy,
		const vk::Image& swapchainImage, const vk::Extent2D& extent)
	{
		// Begin command buffer execution
		vk::CommandBufferBeginInfo cmdBeginInfo = {
			vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
			nullptr, nullptr
		};

		util::CheckResult(cmd.begin(&cmdBeginInfo));

		// Setup pipeline barriers for transitioning image layouts

		vk::ImageSubresourceRange imageSubresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};

		// Offscreen Transition
		vk::ImageMemoryBarrier offscreenMemoryBarrier = {
			vk::AccessFlagBits::eNone, vk::AccessFlagBits::eTransferRead,
			vk::ImageLayout::eShaderReadOnlyOptimal, vk::ImageLayout::eTransferSrcOptimal, {}, {},
			imageToCopy.image, imageSubresourceRange
		};

		cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer,
		                    {}, 0, nullptr, 0, nullptr,
		                    1, &offscreenMemoryBarrier);

		// Swapchain Transition
		vk::ImageMemoryBarrier swapchainMemoryBarrier = {
			vk::AccessFlagBits::eMemoryRead, vk::AccessFlagBits::eTransferWrite,
			vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, {}, {},
			swapchainImage, imageSubresourceRange
		};

		cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer,
		                    {}, 0, nullptr, 0, nullptr,
		                    1, &swapchainMemoryBarrier);

		// Blit (Copy with auto format coversion (RGB to BGR)) offscreen to swapchain image
		vk::Offset3D blitSize =
		{
			static_cast<int32_t>(extent.width),
			static_cast<int32_t>(extent.height),
			1
		};

		std::array<vk::Offset3D, 2> offsets = {};
		offsets[1] = blitSize;

		vk::ImageBlit imageBlitRegion =
		{
			{vk::ImageAspectFlagBits::eColor, 0, 0, 1}, offsets,
			{vk::ImageAspectFlagBits::eColor, 0, 0, 1}, offsets
		};

		cmd.blitImage(imageToCopy.image, vk::ImageLayout::eTransferSrcOptimal,
		              swapchainImage, vk::ImageLayout::eTransferDstOptimal, 1, &imageBlitRegion,
		              vk::Filter::eNearest);

		// Setup pipeline barriers for transitioning image layouts back to default

		// Offscreen Transition
		offscreenMemoryBarrier = {
			vk::AccessFlagBits::eTransferRead, vk::AccessFlagBits::eNone,
			vk::ImageLayout::eTransferSrcOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, {}, {},
			imageToCopy.image, imageSubresourceRange
		};

		cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer,
		                    {}, 0, nullptr, 0, nullptr,
		                    1, &offscreenMemoryBarrier);

		// Swapchain Transition
		swapchainMemoryBarrier = {
			vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eMemoryRead,
			vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::ePresentSrcKHR, {}, {},
			swapchainImage, imageSubresourceRange
		};

		cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer,
		                    {}, 0, nullptr, 0, nullptr,
		                    1, &swapchainMemoryBarrier);

		cmd.end();
	}

	CoreRenderModuleVK::FrameRenderData& CoreRenderModuleVK::GetFrameData(uint8_t frameIdx)
	{
		return mFrameRenderData[frameIdx];
	}

	CoreRenderModuleVK::FrameRenderData& CoreRenderModuleVK::GetCurrentFrameData()
	{
		return GetFrameData(mRenderSubsystem->GetCurrentFrameIdx());
	}

	std::vector<MeshDrawBatch>& CoreRenderModuleVK::GetMeshDrawBatches()
	{
		return mDrawBatches;
	}

	void CoreRenderModuleVK::OnUpdateMesh(entt::registry& registry, entt::entity entity)
	{
		const auto mesh = registry.get<StaticMeshComponent3D>(entity);

		if (mesh.meshID == gInvalidID || mesh.materialID == gInvalidID)
		{
			return;
		}

		mMeshesToLoad.insert(mesh.meshID);
		mRenderSubsystem->GetMaterialRegistry()->AddMaterialInstanceToLoad(mesh.materialID);

		AddRenderable(registry, entity);
	}

	void CoreRenderModuleVK::OnUpdateTransform(entt::registry& registry, entt::entity entity)
	{
		AddRenderable(registry, entity);
	}

	void CoreRenderModuleVK::OnDestroyMeshOrTransform(entt::registry& registry, entt::entity entity)
	{
		mUpdateRenderables = true;
	}

	void CoreRenderModuleVK::OnUpdateLight(entt::registry& registry, entt::entity entity)
	{
		for (auto& frameRenderData : mFrameRenderData)
		{
			frameRenderData.updateGPULightData = true;
		}
	}

	void CoreRenderModuleVK::AddRenderable(entt::registry& registry, entt::entity entity)
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

			mObjectsToUpdate.emplace(id);

			mUpdateRenderables = true;
		}
	}

	void CoreRenderModuleVK::BindCallbacks()
	{
		const auto& enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		
		// Bind callbacks
		const auto registry = enttSubsystem->GetRegistry();

		registry->on_construct<StaticMeshComponent3D>().connect<&CoreRenderModuleVK::OnUpdateMesh>(this);
		registry->on_update<StaticMeshComponent3D>().connect<&CoreRenderModuleVK::OnUpdateMesh>(this);
		registry->on_destroy<StaticMeshComponent3D>().connect<&CoreRenderModuleVK::OnDestroyMeshOrTransform>(this);

		registry->on_construct<TransformComponent2D>().connect<&CoreRenderModuleVK::OnUpdateTransform>(this);
		registry->on_update<TransformComponent2D>().connect<&CoreRenderModuleVK::OnUpdateTransform>(this);
		registry->on_destroy<TransformComponent2D>().connect<&CoreRenderModuleVK::OnDestroyMeshOrTransform>(this);

		registry->on_construct<TransformComponent3D>().connect<&CoreRenderModuleVK::OnUpdateTransform>(this);
		registry->on_update<TransformComponent3D>().connect<&CoreRenderModuleVK::OnUpdateTransform>(this);
		registry->on_destroy<TransformComponent3D>().connect<&CoreRenderModuleVK::OnDestroyMeshOrTransform>(this);

		registry->on_construct<SpotLightComponent3D>().connect<&CoreRenderModuleVK::OnUpdateLight>(this);
		registry->on_destroy<SpotLightComponent3D>().connect<&CoreRenderModuleVK::OnUpdateLight>(this);
		registry->on_destroy<SpotLightComponent3D>().connect<&CoreRenderModuleVK::OnUpdateLight>(this);

		registry->on_construct<DirectionalLightComponent3D>().connect<&CoreRenderModuleVK::OnUpdateLight>(this);
		registry->on_destroy<DirectionalLightComponent3D>().connect<&CoreRenderModuleVK::OnUpdateLight>(this);
		registry->on_destroy<DirectionalLightComponent3D>().connect<&CoreRenderModuleVK::OnUpdateLight>(this);
		
		registry->on_construct<PointLightComponent3D>().connect<&CoreRenderModuleVK::OnUpdateLight>(this);
		registry->on_destroy<PointLightComponent3D>().connect<&CoreRenderModuleVK::OnUpdateLight>(this);
		registry->on_destroy<PointLightComponent3D>().connect<&CoreRenderModuleVK::OnUpdateLight>(this);
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

	void CoreRenderModuleVK::ProcessComponents()
	{
		const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		const auto registry = enttSubsystem->GetRegistry();

		const auto materialRegistry = mRenderSubsystem->GetMaterialRegistry();

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

				const auto& matData = materialRegistry->GetMaterialData(mesh.materialID);

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

				const auto& matData = materialRegistry->GetMaterialData(mesh.materialID);

				mRenderables.emplace_back(node_id, mesh.meshID, matData.baseMaterialID, mesh.subMeshIdx);

				if (!mCachedObjectData.Contains(node_id))
				{
					mCachedObjectData.Emplace(node_id, GPUObjectData());
				}
			}

			std::sort(mRenderables.begin(), mRenderables.end());

			for (auto& frameData : mFrameRenderData)
			{
				frameData.updateGPUObjectData = true;
			}
			
			mUpdateRenderables = false;
		}
	}

	void CoreRenderModuleVK::UpdateTextureDescriptors()
	{
		if (mInitialized && GetCurrentFrameData().textureDescriptorNeedsUpdated)
		{
			std::vector<vk::DescriptorImageInfo> textureImageInfos;
			BuildTextureDescriptorInfo(mTexData, textureImageInfos);

			util::DescriptorBuilder::Begin(mRenderSubsystem->GetDescriptorLayoutCache(), mRenderSubsystem->GetDescriptorAllocator())
				.UpdateImages(1, textureImageInfos.size(), textureImageInfos.data(),
							  vk::DescriptorType::eCombinedImageSampler)
				.Update(GetCurrentFrameData().materialDescriptor);

			GetCurrentFrameData().textureDescriptorNeedsUpdated = false;
		}
	}

	void CoreRenderModuleVK::BuildTextureDescriptorInfo(MappedVector<UUID, TextureDataVK>& textureData,
		std::vector<vk::DescriptorImageInfo>& textureImageInfos) const
	{
		textureImageInfos.clear();
		textureImageInfos.reserve(textureData.Size());

		const auto& textureManager = mRenderSubsystem->GetTextureManager();

		int idx = 0;
		for (auto& texData : textureData)
		{
			const auto& texture = textureManager->GetTexture(texData.id);
			
			vk::DescriptorImageInfo textureImageInfo = {
				texData.sampler, texture.imageView, vk::ImageLayout::eShaderReadOnlyOptimal
			};
			textureImageInfos.push_back(textureImageInfo);

			texData.idx = idx;
			idx++;
		}
	}

	void CoreRenderModuleVK::PrepareSceneData()
	{
		PrepareMaterialData();
		PrepareObjectData();
		PrepareLightData();
	}

	void CoreRenderModuleVK::PrepareMaterialData()
	{
		const auto& materialRegistry = mRenderSubsystem->GetMaterialRegistry();
		const auto& resourceManager = mRenderSubsystem->GetResourceManager();
		
		if (GetCurrentFrameData().updateGPUMaterialData)
		{
			std::vector<GPUMaterialInstanceData> materialData;
			materialData.reserve(materialRegistry->GetAllMaterialData().Size());

			int idx = 0;
			for (auto& matData : materialRegistry->GetAllMaterialData())
			{
				// Update cached material data
				for (int i = 0; i < gNumTexturesPerMat; ++i)
				{
					if (matData.texIDs[i] != 0)
					{
						materialRegistry->GetCachedMaterialData(matData.assetId).texIndices[i] = mTexData[matData.texIDs[i]].idx;
					}
				}

				materialData.push_back(materialRegistry->GetCachedMaterialData(matData.assetId));

				matData.idx = idx;

				idx++;
			}

			util::CopyCPUDataIntoGPUBufferParams params;
			params.dataSize = sizeof(GPUMaterialInstanceData) * materialData.size();
			params.dstBuffer = resourceManager->GetBuffer(mMaterialInstanceBufferID);
			params.srcData = materialData.data();
			util::CopyCPUDataIntoGPUBuffer(mRenderSubsystem, params);

			GetCurrentFrameData().updateGPUMaterialData = false;
		}
	}

	void CoreRenderModuleVK::PrepareObjectData()
	{
		const auto& resourceManager = mRenderSubsystem->GetResourceManager();
		const auto& materialRegistry = mRenderSubsystem->GetMaterialRegistry();
		
		if (!mObjectsToUpdate.empty())
		{
			const auto enkiTSSubsystem = mEngine->GetSubsystem<core::EnkiTSSubsystem>();

			// Calculate t value for rendering interpolated position
			const double t = mEngine->GetAccumulatedTime() / mEngine->GetTimeStepFixed();

			std::vector<UUID> objectsToRefresh;
			objectsToRefresh.reserve(mObjectsToUpdate.size());

			for (const auto id : mObjectsToUpdate)
			{
				objectsToRefresh.push_back(id);
			}

			const auto numObjectsToUpdate = objectsToRefresh.size();

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

			enki::TaskSet task(numObjectsToUpdate, [&](enki::TaskSetPartition range, uint32_t threadnum)
			{
				for (uint32_t objectIdx = range.start; objectIdx < range.end; objectIdx++)
				{
					const auto entityID = objectsToRefresh[objectIdx];
					const auto entity = enttSubsystem->GetEntity(entityID);
					auto* node = sceneGraph->GetNode(entityID);

					// Position
#ifdef PFN_DOUBLE_PRECISION
					Vector3d position = { 0.0 };
#else
					Vector3f position = { 0.0f };
#endif

					{
						if (auto* transformNode3D = dynamic_cast<TransformNode3D*>(node); transformNode3D)
						{
							position = transformNode3D->GetGlobalTransform().position;
						}
						else
						{
							position = registry->get<TransformComponent3D>(entity).position;
						}
							
						if (registry->any_of<physics::VelocityComponent3D>(entity))
						{
							const auto& velocity = registry->get<physics::VelocityComponent3D>(entity);

#ifdef PFN_DOUBLE_PRECISION
							Vector3d interpolatedPosition = {};
#else
							Vector3f interpolatedPosition = {};
#endif
							interpolatedPosition = position + velocity.linear * mEngine->GetTimeStepFixed();
								
							position = maths::Lerp(position, interpolatedPosition, t);
						}
					}

					// Orientation
					
					maths::Quat orientation;
					{
						if (auto* transformNode3D = dynamic_cast<TransformNode3D*>(node); transformNode3D)
						{
							orientation = transformNode3D->GetGlobalTransform().orientationQuat;
						}
						else
						{
							orientation = registry->get<TransformComponent3D>(entity).orientationQuat;
						}
					}

					// Scale

					Vector3f scale;
					{
						if (auto* transformNode3D = dynamic_cast<TransformNode3D*>(node); transformNode3D)
						{
							scale = transformNode3D->GetGlobalTransform().scale;
						}
						else
						{
							scale = registry->get<TransformComponent3D>(entity).scale;
						}
					}

					GPUObjectData object = {};

					util::UpdateModelTransform(position, orientation, scale, object.model);

					const auto& mesh = registry->get<StaticMeshComponent3D>(entity);
					object.matIdx = materialRegistry->GetMaterialData(mesh.materialID).idx;

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

			mObjectsToUpdate.clear();
		}

		if (GetCurrentFrameData().updateGPUObjectData)
		{
			std::vector<GPUObjectData> objects = {};
			objects.reserve(gMaxObjects);

			for (const auto& renderable : mRenderables)
			{
				objects.emplace_back(mCachedObjectData[renderable.entityID]);
			}

			util::CopyCPUDataIntoGPUBufferParams params;
			params.dataSize = sizeof(GPUObjectData) * objects.size();
			params.dstBuffer = resourceManager->GetBuffer(mObjectBufferID);
			params.srcData = objects.data();
			util::CopyCPUDataIntoGPUBuffer(mRenderSubsystem, params);

			GetCurrentFrameData().updateGPUObjectData = false;
		}
	}

	void CoreRenderModuleVK::PrepareLightData()
	{
		const auto& resourceManager = mRenderSubsystem->GetResourceManager();
		
		if (GetCurrentFrameData().updateGPULightData)
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
	
					pointLights[p].positionShadowIndex.w = shadow.shadowIdx;
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

				// PUFFIN_TODO - Re-enable when re-implementing shadows for render modules
				/*if (registry->any_of<ShadowCasterComponent3D>(entity))
				{
					const auto& shadow = registry->get<ShadowCasterComponent3D>(entity);

					spotLights[s].positionShadowIndex.w = shadow.shadowIdx;
				}*/

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

				// PUFFIN_TODO - Re-enable when re-implementing shadows for render modules
				/*if (registry->any_of<ShadowCasterComponent3D>(entity))
				{
					const auto& shadow = registry->get<ShadowCasterComponent3D>(entity);
	
					dirLights[d].positionShadowIndex.w = shadow.shadowIdx;
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
			params.dstBuffer = resourceManager->GetBuffer(mPointLightBufferID);
			params.srcData = pointLights.data();

			util::CopyCPUDataIntoGPUBuffer(mRenderSubsystem, params);

			// Copy spot light data to buffer
			params.dataSize = spotLights.size() * sizeof(GPUSpotLightData);
			params.dstBuffer = resourceManager->GetBuffer(mSpotLightBufferID);
			params.srcData = spotLights.data();

			util::CopyCPUDataIntoGPUBuffer(mRenderSubsystem, params);

			// Copy directional light data to buffer
			params.dataSize = dirLights.size() * sizeof(GPUDirLightData);
			params.dstBuffer = resourceManager->GetBuffer(mDirLightBufferID);
			params.srcData = dirLights.data();

			util::CopyCPUDataIntoGPUBuffer(mRenderSubsystem, params);

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

			GetCurrentFrameData().updateGPULightData = false;
		}
	}

	void CoreRenderModuleVK::BuildIndirectCommands()
	{
		const auto& resourceManager = mRenderSubsystem->GetResourceManager();
		const auto& materialRegistry = mRenderSubsystem->GetMaterialRegistry();
		const auto& unifiedGeometryBuffer = mRenderSubsystem->GetUnifiedGeometryBuffer();
		
		if (!mRenderables.empty())
		{
			std::vector<vk::DrawIndexedIndirectCommand> indirectCmds = {};
			indirectCmds.resize(gMaxObjects);

			mDrawBatches.clear();
			mDrawBatches.reserve(materialRegistry->GetAllMaterialData().Size());

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

			indirectCmds[cmdIdx].vertexOffset = unifiedGeometryBuffer->MeshVertexOffset(currentMeshID, currentSubMeshIdx);
			indirectCmds[cmdIdx].firstIndex = unifiedGeometryBuffer->MeshIndexOffset(currentMeshID, currentSubMeshIdx);
			indirectCmds[cmdIdx].indexCount = unifiedGeometryBuffer->MeshIndexCount(currentMeshID, currentSubMeshIdx);
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

					indirectCmds[cmdIdx].vertexOffset = unifiedGeometryBuffer->MeshVertexOffset(currentMeshID, currentSubMeshIdx);
					indirectCmds[cmdIdx].firstIndex = unifiedGeometryBuffer->MeshIndexOffset(currentMeshID, currentSubMeshIdx);
					indirectCmds[cmdIdx].indexCount = unifiedGeometryBuffer->MeshIndexCount(currentMeshID, currentSubMeshIdx);
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
			params.dstBuffer = resourceManager->GetBuffer(mIndirectDrawBufferID);
			params.srcData = indirectCmds.data();
			util::CopyCPUDataIntoGPUBuffer(mRenderSubsystem, params);
		}
	}
}
