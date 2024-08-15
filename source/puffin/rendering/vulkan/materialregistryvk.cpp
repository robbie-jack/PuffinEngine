#include "puffin/rendering/vulkan/materialregistryvk.h"

#include "puffin/assets/assetregistry.h"
#include "puffin/assets/materialasset.h"
#include "puffin/assets/shaderasset.h"
#include "puffin/rendering/vulkan/pipelinevk.h"
#include "puffin/rendering/vulkan/rendersubsystemvk.h"

namespace puffin::rendering
{
	MaterialRegistryVK::MaterialRegistryVK(RenderSubystemVK* render_system) : m_render_system(render_system)
	{
	}

	MaterialRegistryVK::~MaterialRegistryVK()
	{
		for (auto& [id, mat] : m_mats)
		{
			m_render_system->device().destroyPipeline(mat.pipeline.get());
			m_render_system->device().destroyPipelineLayout(mat.pipelineLayout.get());

			mat.pipeline = {};
			mat.pipelineLayout = {};
		}

		m_cached_material_data.clear();
		m_mat_data.clear();
		m_mats.clear();

		m_render_system = nullptr;
	}

	void MaterialRegistryVK::register_material_instance(const PuffinID& id)
	{
		if (id != gInvalidID)
		{
			m_materials_instances_to_load.insert(id);
		}

		// PUFFIN_TODO - Load Material Instance Assets Asynchronously
	}

	void MaterialRegistryVK::update()
	{
		mMaterialDataNeedsUploaded = false;

		// Load Material Instances
		for (const auto matInstID : m_materials_instances_to_load)
		{
			if (matInstID != gInvalidID && !m_mat_data.contains(matInstID))
			{
				m_mat_data.emplace(matInstID, MaterialDataVK());

				load_material_instance(matInstID, m_mat_data.at(matInstID));

				mMaterialDataNeedsUploaded = true;
			}
		}

		m_materials_instances_to_load.clear();

		if (mMaterialDataNeedsUploaded)
		{
			m_mat_data.sort();
		}

		// Load Materials
		for (const auto matID : m_materials_to_load)
		{
			init_material_pipeline(matID);
		}

		m_materials_to_load.clear();
	}

	bool MaterialRegistryVK::load_material_instance(PuffinID matID, MaterialDataVK& matData)
	{
		const auto matAsset = assets::AssetRegistry::Get()->GetAsset<assets::MaterialInstanceAsset>(matID);

		if (matAsset && matAsset->Load())
		{
			matData.assetId = matID;
			matData.baseMaterialID = matAsset->GetBaseMaterialID();

			GPUMaterialInstanceData matInstData;

			int i = 0;
			for (const auto& idx : matAsset->GetTexIDs())
			{
				matData.texIDs[i] = idx;
				matInstData.tex_indices[i] = 0;

				++i;
			}

			i = 0;
			for (const auto& data : matAsset->GetData())
			{
				matInstData.data[i] = data;

				++i;
			}

			m_cached_material_data.emplace(matID, matInstData);

			for (const auto& texID : matData.texIDs)
			{
				if (texID != gInvalidID)
				{
					m_render_system->register_texture(texID);
				}
			}

			if (matData.baseMaterialID != gInvalidID)
			{
				m_materials_to_load.insert(matData.baseMaterialID);
			}

			matAsset->Unload();

			return true;
		}

		return false;
	}

	void MaterialRegistryVK::init_material_pipeline(PuffinID matID)
	{
		if (!m_mat_data.contains(matID))
		{
			const auto matAsset = assets::AssetRegistry::Get()->GetAsset<assets::MaterialAsset>(matID);

			if (matAsset && matAsset->Load())
			{
				const auto vertShaderAsset = assets::AssetRegistry::Get()->GetAsset<assets::ShaderAsset>(matAsset->GetVertexShaderID());
				const auto fragShaderAsset = assets::AssetRegistry::Get()->GetAsset<assets::ShaderAsset>(matAsset->GetFragmentShaderID());

				if (vertShaderAsset && vertShaderAsset->Load() && fragShaderAsset && fragShaderAsset->Load())
				{
					const auto vertMod = util::ShaderModule{ m_render_system->device(), vertShaderAsset->GetCode() };
					const auto fragMod = util::ShaderModule{ m_render_system->device(), fragShaderAsset->GetCode() };

					m_mats.emplace(matID, MaterialVK());

					vk::PushConstantRange range = { vk::ShaderStageFlagBits::eVertex, 0, sizeof(GPUVertexShaderPushConstant) };

					MaterialVK& mat = m_mats[matID];
					mat.matID = matID;

					util::PipelineLayoutBuilder plb{};
					mat.pipelineLayout = plb
						.descriptorSetLayout(m_render_system->static_render_data().global_set_layout)
						.pushConstantRange(range)
						.createUnique(m_render_system->device());

					vk::PipelineDepthStencilStateCreateInfo depthStencilInfo = { {}, true, true,
						vk::CompareOp::eLessOrEqual, false, false, {}, {}, 0.0f, 1.0f };

					vk::PipelineRenderingCreateInfoKHR pipelineRenderInfo = {
						0, m_render_system->offscreen_data().image_format, m_render_system->offscreen_data().alloc_depth_image.format
					};

					util::PipelineBuilder pb{ m_render_system->window_size().width, m_render_system->window_size().height };
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
						.createUnique(m_render_system->device(), m_render_system->pipeline_cache(), *mat.pipelineLayout, nullptr);

					m_render_system->device().destroyShaderModule(vertMod.module());
					m_render_system->device().destroyShaderModule(fragMod.module());

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
