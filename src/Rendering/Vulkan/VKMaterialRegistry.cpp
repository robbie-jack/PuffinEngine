#include "Rendering/Vulkan/VKMaterialRegistry.h"

#include "Assets/AssetRegistry.h"
#include "Assets/MaterialAsset.h"
#include "Assets/ShaderAsset.h"
#include "Rendering/Vulkan/VKRenderSystem.h"

namespace puffin::rendering
{
	void VKMaterialRegistry::init(const std::shared_ptr<VKRenderSystem>& renderSystem)
	{
		mRenderSystem = renderSystem;

		mMats.resize(gMaxUniqueMaterials);
	}

	void VKMaterialRegistry::registerMaterialInstance(const PuffinID& id)
	{
		mMaterialsInstancesToLoad.insert(id);

		// PUFFIN_TODO - Load Material Instance Assets Asynchronously
	}

	void VKMaterialRegistry::update()
	{
		mMaterialDataNeedsUploaded = false;

		// Load Material Instances
		for (const auto matInstID : mMaterialsInstancesToLoad)
		{
			if (!mMatData.contains(matInstID))
			{
				MaterialDataVK matData;

				loadMaterialInstance(matInstID, matData);

				mMatData.insert(matInstID, matData);

				mMaterialDataNeedsUploaded = true;
			}
		}

		mMaterialsInstancesToLoad.clear();

		if (mMaterialDataNeedsUploaded)
		{
			mMatData.sortBubble();
		}

		// Load Materials
		for (const auto matID : mMaterialsToLoad)
		{
			initMaterialPipeline(matID);
		}

		mMaterialsToLoad.clear();
	}

	bool VKMaterialRegistry::loadMaterialInstance(PuffinID matID, MaterialDataVK& matData)
	{
		const auto matAsset = assets::AssetRegistry::get()->getAsset<assets::MaterialInstanceAsset>(matID);

		if (matAsset && matAsset->load())
		{
			matData.assetId = matID;
			matData.baseMaterialID = matAsset->getBaseMaterialID();

			GPUMaterialInstanceData matInstData;

			int i = 0;
			for (const auto& idx : matAsset->getTexIDs())
			{
				matData.texIDs[i] = idx;
				matInstData.texIndices[i] = 0;

				++i;
			}

			i = 0;
			for (const auto& data : matAsset->getData())
			{
				matInstData.data[i] = data;

				++i;
			}

			mCachedMaterialData.emplace(matID, matInstData);

			for (const auto& texID : matData.texIDs)
			{
				if (texID != gInvalidID)
				{
					mRenderSystem->registerTexture(texID);
				}
			}

			if (matData.baseMaterialID != gInvalidID)
			{
				mMaterialsToLoad.insert(matData.baseMaterialID);
			}

			matAsset->unload();

			return true;
		}

		return false;
	}

	void VKMaterialRegistry::initMaterialPipeline(PuffinID matID)
	{
		if (!mMatData.contains(matID))
		{
			const auto matAsset = assets::AssetRegistry::get()->getAsset<assets::MaterialAsset>(matID);

			if (matAsset && matAsset->load())
			{
				const auto vertShaderAsset = assets::AssetRegistry::get()->getAsset<assets::ShaderAsset>(matAsset->getVertexShaderID());
				const auto fragShaderAsset = assets::AssetRegistry::get()->getAsset<assets::ShaderAsset>(matAsset->getFragmentShaderID());

				if (vertShaderAsset && vertShaderAsset->load() && fragShaderAsset && fragShaderAsset->load())
				{
					const auto vertMod = util::ShaderModule{ mRenderSystem->device(), vertShaderAsset->code() };
					const auto fragMod = util::ShaderModule{ mRenderSystem->device(), fragShaderAsset->code() };

					mMats.insert(matID);

					MaterialVK& mat = mMats[matID];
					mat.matID = matID;

					util::PipelineLayoutBuilder plb{};
					mat.pipelineLayout = plb
						.descriptorSetLayout(mRenderSystem->staticRenderData().globalSetLayout)
						.createUnique(mRenderSystem->device());

					vk::PipelineDepthStencilStateCreateInfo depthStencilInfo = { {}, true, true,
						vk::CompareOp::eLessOrEqual, false, false, {}, {}, 0.0f, 1.0f };

					vk::PipelineRenderingCreateInfoKHR pipelineRenderInfo = {
						0, mRenderSystem->offscreenData().imageFormat, mRenderSystem->offscreenData().allocDepthImage.format
					};

					util::PipelineBuilder pb{ mRenderSystem->windowSize().width, mRenderSystem->windowSize().height };
					mat.pipeline = pb
						// Define dynamic state which can change each frame (currently viewport and scissor size)
						.dynamicState(vk::DynamicState::eViewport)
						.dynamicState(vk::DynamicState::eScissor)
						// Define vertex/fragment shaders
						.shader(vk::ShaderStageFlagBits::eVertex, vertMod)
						.shader(vk::ShaderStageFlagBits::eFragment, fragMod)
						.depthStencilState(depthStencilInfo)
						// Define vertex binding/attributes
						.vertexLayout(VertexPNTV32::getLayoutVK())
						// Add rendering info struct
						.addPNext(&pipelineRenderInfo)
						// Create pipeline
						.createUnique(mRenderSystem->device(), mRenderSystem->pipelineCache(), *mat.pipelineLayout, nullptr);

					//mDevice.destroyShaderModule(vertMod.module());
					//mDevice.destroyShaderModule(fragMod.module());

					mRenderSystem->deletionQueue().pushFunction([&]()
					{
						mat.pipeline = {};
						mat.pipelineLayout = {};
					});
				}
			}
		}
	}
}
