#include "puffin/rendering/vulkan/vk_material_registry.h"

#include "puffin/assets/asset_registry.h"
#include "puffin/assets/material_asset.h"
#include "puffin/assets/shader_asset.h"
#include "puffin/rendering/vulkan/vk_pipeline.h"
#include "puffin/rendering/vulkan/vk_render_system.h"

namespace puffin::rendering
{
	void VKMaterialRegistry::init(const std::shared_ptr<RenderSystemVK>& renderSystem)
	{
		mRenderSystem = renderSystem;
	}

	void VKMaterialRegistry::registerMaterialInstance(const PuffinID& id)
	{
		if (id != gInvalidID)
		{
			mMaterialsInstancesToLoad.insert(id);
		}

		// PUFFIN_TODO - Load Material Instance Assets Asynchronously
	}

	void VKMaterialRegistry::update()
	{
		mMaterialDataNeedsUploaded = false;

		// Load Material Instances
		for (const auto matInstID : mMaterialsInstancesToLoad)
		{
			if (matInstID != gInvalidID && !mMatData.contains(matInstID))
			{
				mMatData.emplace(matInstID, MaterialDataVK());

				loadMaterialInstance(matInstID, mMatData.at(matInstID));

				mMaterialDataNeedsUploaded = true;
			}
		}

		mMaterialsInstancesToLoad.clear();

		if (mMaterialDataNeedsUploaded)
		{
			mMatData.sort();
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
					mRenderSystem->register_texture(texID);
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

					mMats.emplace(matID, MaterialVK());

					vk::PushConstantRange range = { vk::ShaderStageFlagBits::eVertex, 0, sizeof(GPUDrawPushConstant) };

					MaterialVK& mat = mMats[matID];
					mat.matID = matID;

					util::PipelineLayoutBuilder plb{};
					mat.pipelineLayout = plb
						.descriptorSetLayout(mRenderSystem->static_render_data().global_set_layout)
						.pushConstantRange(range)
						.createUnique(mRenderSystem->device());

					vk::PipelineDepthStencilStateCreateInfo depthStencilInfo = { {}, true, true,
						vk::CompareOp::eLessOrEqual, false, false, {}, {}, 0.0f, 1.0f };

					vk::PipelineRenderingCreateInfoKHR pipelineRenderInfo = {
						0, mRenderSystem->offscreen_data().imageFormat, mRenderSystem->offscreen_data().allocDepthImage.format
					};

					util::PipelineBuilder pb{ mRenderSystem->window_size().width, mRenderSystem->window_size().height };
					mat.pipeline = pb
						// Define dynamic state which can change each frame (currently viewport and scissor size)
						.dynamicState(vk::DynamicState::eViewport)
						.dynamicState(vk::DynamicState::eScissor)
						// Define vertex/fragment shaders
						.shader(vk::ShaderStageFlagBits::eVertex, vertMod)
						.shader(vk::ShaderStageFlagBits::eFragment, fragMod)
						.depthStencilState(depthStencilInfo)
						// Add rendering info struct
						.addPNext(&pipelineRenderInfo)
						// Create pipeline
						.createUnique(mRenderSystem->device(), mRenderSystem->pipeline_cache(), *mat.pipelineLayout, nullptr);

					//mDevice.destroyShaderModule(vertMod.module());
					//mDevice.destroyShaderModule(fragMod.module());

					mRenderSystem->deletion_queue().pushFunction([&]()
					{
						mat.pipeline = {};
						mat.pipelineLayout = {};
					});
				}
			}
		}
	}
}
