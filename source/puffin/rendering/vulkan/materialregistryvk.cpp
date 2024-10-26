#include "puffin/rendering/vulkan/materialregistryvk.h"

#include "puffin/assets/assetregistry.h"
#include "puffin/assets/materialasset.h"
#include "puffin/assets/shaderasset.h"
#include "puffin/rendering/vulkan/pipelinevk.h"
#include "puffin/rendering/vulkan/rendersubsystemvk.h"

namespace puffin::rendering
{
	MaterialRegistryVK::MaterialRegistryVK(RenderSubsystemVK* renderSystem) : mRenderSystem(renderSystem)
	{
	}

	MaterialRegistryVK::~MaterialRegistryVK()
	{
		for (auto& [id, mat] : mMats)
		{
			mRenderSystem->GetDevice().destroyPipeline(mat.pipeline.get());
			mRenderSystem->GetDevice().destroyPipelineLayout(mat.pipelineLayout.get());

			mat.pipeline = {};
			mat.pipelineLayout = {};
		}

		mCachedMaterialData.Clear();
		mMatData.Clear();
		mMats.clear();

		mRenderSystem = nullptr;
	}

	void MaterialRegistryVK::RegisterMaterialInstance(const UUID& id)
	{
		if (id != gInvalidID)
		{
			mMaterialsInstancesToLoad.insert(id);
		}

		// PUFFIN_TODO - Load Material Instance Assets Asynchronously
	}

	void MaterialRegistryVK::Update()
	{
		mMaterialDataNeedsUploaded = false;

		// Load Material Instances
		for (const auto matInstID : mMaterialsInstancesToLoad)
		{
			if (matInstID != gInvalidID && !mMatData.Contains(matInstID))
			{
				mMatData.Emplace(matInstID, MaterialDataVK());

				LoadMaterialInstance(matInstID, mMatData.At(matInstID));

				mMaterialDataNeedsUploaded = true;
			}
		}

		mMaterialsInstancesToLoad.clear();

		if (mMaterialDataNeedsUploaded)
		{
			mMatData.Sort();
		}

		// Load Materials
		for (const auto matID : mMaterialsToLoad)
		{
			InitMaterialPipeline(matID);
		}

		mMaterialsToLoad.clear();
	}

	bool MaterialRegistryVK::GetMaterialDataNeedsUploaded() const
	{
		return mMaterialDataNeedsUploaded;
	}

	MaterialDataVK& MaterialRegistryVK::GetMaterialData(const UUID& id)
	{
		return mMatData[id];
	}

	MappedVector<UUID, MaterialDataVK>& MaterialRegistryVK::GetAllMaterialData()
	{
		return mMatData;
	}

	MaterialVK& MaterialRegistryVK::GetMaterial(const UUID& id)
	{
		return mMats[id];
	}

	GPUMaterialInstanceData& MaterialRegistryVK::GetCachedMaterialData(const UUID& id)
	{
		return mCachedMaterialData[id];
	}

	bool MaterialRegistryVK::LoadMaterialInstance(UUID matID, MaterialDataVK& matData)
	{
		if (const auto matAsset = assets::AssetRegistry::Get()->GetAsset<assets::MaterialInstanceAsset>(matID); matAsset && matAsset->Load())
		{
			matData.assetId = matID;
			matData.baseMaterialID = matAsset->GetBaseMaterialID();

			GPUMaterialInstanceData matInstData;

			int i = 0;
			for (const auto& idx : matAsset->GetTexIDs())
			{
				matData.texIDs[i] = idx;
				matInstData.texIndices[i] = 0;

				++i;
			}

			i = 0;
			for (const auto& data : matAsset->GetData())
			{
				matInstData.data[i] = data;

				++i;
			}

			mCachedMaterialData.Emplace(matID, matInstData);

			for (const auto& texID : matData.texIDs)
			{
				if (texID != gInvalidID)
				{
					mRenderSystem->RegisterTexture(texID);
				}
			}

			if (matData.baseMaterialID != gInvalidID)
			{
				mMaterialsToLoad.insert(matData.baseMaterialID);
			}

			matAsset->Unload();

			return true;
		}

		return false;
	}

	void MaterialRegistryVK::InitMaterialPipeline(UUID matID)
	{
		if (!mMatData.Contains(matID))
		{
			if (const auto matAsset = assets::AssetRegistry::Get()->GetAsset<assets::MaterialAsset>(matID); matAsset && matAsset->Load())
			{
				const auto vertShaderAsset = assets::AssetRegistry::Get()->GetAsset<assets::ShaderAsset>(matAsset->GetVertexShaderID());
				const auto fragShaderAsset = assets::AssetRegistry::Get()->GetAsset<assets::ShaderAsset>(matAsset->GetFragmentShaderID());

				if (vertShaderAsset && vertShaderAsset->Load() && fragShaderAsset && fragShaderAsset->Load())
				{
					const auto vertMod = util::ShaderModule{ mRenderSystem->GetDevice(), vertShaderAsset->GetCode() };
					const auto fragMod = util::ShaderModule{ mRenderSystem->GetDevice(), fragShaderAsset->GetCode() };

					mMats.emplace(matID, MaterialVK());

					constexpr vk::PushConstantRange range = { vk::ShaderStageFlagBits::eVertex, 0, sizeof(GPUVertexShaderPushConstant) };

					MaterialVK& mat = mMats[matID];
					mat.matID = matID;

					util::PipelineLayoutBuilder plb{};
					mat.pipelineLayout = plb
						.DescriptorSetLayout(mRenderSystem->GetGlobalRenderData().lightSetLayout)
						.PushConstantRange(range)
						.CreateUnique(mRenderSystem->GetDevice());

					constexpr vk::PipelineDepthStencilStateCreateInfo depthStencilInfo = { {}, true, true,
						vk::CompareOp::eLessOrEqual, false, false, {}, {}, 0.0f, 1.0f };

					vk::PipelineRenderingCreateInfoKHR pipelineRenderInfo = {
						0, mRenderSystem->GetOffscreenData().imageFormat, mRenderSystem->GetOffscreenData().allocDepthImage.format
					};

					util::PipelineBuilder pb{ mRenderSystem->GetSwapchainExtent().width, mRenderSystem->GetSwapchainExtent().height };
					mat.pipeline = pb
						// Define dynamic state which can change each frame (currently viewport and scissor size)
						.DynamicState(vk::DynamicState::eViewport)
						.DynamicState(vk::DynamicState::eScissor)
						// Define vertex/fragment shaders
						.Shader(vk::ShaderStageFlagBits::eVertex, vertMod)
						.Shader(vk::ShaderStageFlagBits::eFragment, fragMod)
						.DepthStencilState(depthStencilInfo)
						// Add rendering info struct
						.AddPNext(&pipelineRenderInfo)
						// Create pipeline
						.CreateUnique(mRenderSystem->GetDevice(), mRenderSystem->GetPipelineCache(), *mat.pipelineLayout, nullptr);

					mRenderSystem->GetDevice().destroyShaderModule(vertMod.Module());
					mRenderSystem->GetDevice().destroyShaderModule(fragMod.Module());

					/*m_render_system->deletion_queue().pushFunction([&]()
					{
						mat.pipeline = {};
						mat.pipelineLayout = {};
					});*/
				}
			}
		}
	}
}
