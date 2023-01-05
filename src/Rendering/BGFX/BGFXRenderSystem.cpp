#include "Rendering/BGFX/BGFXRenderSystem.hpp"

#include "bgfx/bgfx.h"
#include "bx/math.h"

#include "Engine/Engine.hpp"
#include "Window/WindowSubsystem.hpp"

#include "GLFW/glfw3.h"

#if PFN_PLATFORM_WIN32
	#define GLFW_EXPOSE_NATIVE_WIN32
#else
X_PLATFORM_LINUX || BX_PLATFORM_BSD
	#define GLFW_EXPOSE_NATIVE_X11
#endif

#include "Assets/AssetRegistry.h"
#include "GLFW/glfw3native.h"

#include "Components/TransformComponent.h"
#include "Components/Rendering/MeshComponent.h"
#include "Components/Rendering/CameraComponent.h"
#include "Assets/AssetRegistry.h"

#include <vector>

namespace Puffin::Rendering::BGFX
{
	void BGFXRenderSystem::Init()
	{
        bgfx::PlatformData pd;
        GLFWwindow* window = m_engine->GetSubsystem<Window::WindowSubsystem>()->GetPrimaryWindow();

#if PFN_PLATFORM_WIN32

        pd.ndt = NULL;
        pd.nwh = glfwGetWin32Window(window);

#else

        pd.ndt = glfwGetX11Display();
        pd.nwh = (void*)glfwGetX11Window(window);

        // Set Wayland instead of X11
        // pd.ndt = glfwGetWaylandDisplay();

#endif

        bgfx::Init bgfxInit;

        // Set Renderer API to Vulkan
        bgfxInit.type = bgfx::RendererType::Vulkan;

        int width, height;
        glfwGetWindowSize(window, &width, &height);

        bgfxInit.resolution.width = static_cast<uint32_t>(width);
        bgfxInit.resolution.height = static_cast<uint32_t>(height);
        bgfxInit.resolution.reset = BGFX_RESET_VSYNC;
        bgfxInit.platformData = pd;
        bgfx::init(bgfxInit);

        bgfx::setDebug(BGFX_DEBUG_NONE);

        bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x00B5E2FF, 1.0f, 0);
        bgfx::setViewRect(0, 0, 0, static_cast<uint16_t>(width), static_cast<uint16_t>(height));

        // Create Static Vertex Buffer
        m_vbh = bgfx::createVertexBuffer(bgfx::makeRef(s_cubeVertices, sizeof(s_cubeVertices)), s_layoutVertexPC32);

        // Create Static Index Buffer
        m_ibh = bgfx::createIndexBuffer(bgfx::makeRef(s_cubeTriList, sizeof(s_cubeTriList)));

        /*m_vsh = LoadShader("C:\\Projects\\PuffinEngine\\bin\\spirv\\vs_cubes.bin");
        m_fsh = LoadShader("C:\\Projects\\PuffinEngine\\bin\\spirv\\fs_cubes.bin");*/

        m_vsh = LoadShader("C:\\Projects\\PuffinEngine\\bin\\spirv\\forward_shading\\vs_forward_shading.bin");
        m_fsh = LoadShader("C:\\Projects\\PuffinEngine\\bin\\spirv\\forward_shading\\fs_forward_shading.bin");

        m_program = bgfx::createProgram(m_vsh, m_fsh, true);
	}

	void BGFXRenderSystem::PreStart()
	{
        InitComponents();
	}

	void BGFXRenderSystem::Update()
	{
		UpdateComponents();

        Draw();
	}

	void BGFXRenderSystem::Stop()
	{
		CleanupComponents();
	}

	void BGFXRenderSystem::Cleanup()
	{
        bgfx::destroy(m_ibh);
		bgfx::destroy(m_vbh);

        bgfx::destroy(m_program);

		bgfx::shutdown();
	}

	void BGFXRenderSystem::InitComponents()
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

	void BGFXRenderSystem::UpdateComponents()
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

            if (entity->GetComponentFlag<MeshComponent, FlagDeleted>())
            {
				CleanupMeshComponent(entity);

                entity->RemoveComponent<MeshComponent>();
            }
        }
	}

	void BGFXRenderSystem::CleanupComponents()
	{
        std::vector<std::shared_ptr<ECS::Entity>> meshEntities;
        ECS::GetEntities<TransformComponent, MeshComponent>(m_world, meshEntities);
        for (const auto& entity : meshEntities)
        {
            CleanupMeshComponent(entity);
        }
	}

	void BGFXRenderSystem::Draw()
	{
        GLFWwindow* window = m_engine->GetSubsystem<Window::WindowSubsystem>()->GetPrimaryWindow();

        int width, height;
        glfwGetWindowSize(window, &width, &height);

        // Dummy draw call to make sure view 0 is cleared
        bgfx::touch(0);

        // Setup View/Projection Matrices
        const bx::Vec3 at = { 0.0f, 0.0f, 0.0f };
        const bx::Vec3 eye = { 0.0f, 0.0f, -5.0f };

        float view[16];
        bx::mtxLookAt(view, eye, at);

        float proj[16];
        bx::mtxProj(proj, 60.0f, width / height, 0.1f, 100.0f, bgfx::getCaps()->homogeneousDepth);

        bgfx::setViewTransform(0, view, proj);

        // Setup Transform
        float mtx[16];
        bx::mtxIdentity(mtx);
        //bx::mtxRotateY(mtx, m_frameCounter * 0.01f);
        bgfx::setTransform(mtx);

        // Set Vertex/Index Buffers
        bgfx::setVertexBuffer(0, m_vbh);
        bgfx::setIndexBuffer(m_ibh);

        // Submit Program
        bgfx::submit(0, m_program);

        // Advance to next frame
        bgfx::frame();

        m_frameCounter++;
	}

	void BGFXRenderSystem::InitMeshComponent(std::shared_ptr<ECS::Entity> entity)
	{
        auto& mesh = entity->GetComponent<MeshComponent>();

        if (!m_meshData.Contains(mesh.meshAssetID))
        {
	        LoadAndInitMesh(mesh.meshAssetID);

            m_meshData[mesh.meshAssetID].entities.insert(entity->ID());
        }

        if (!m_meshDrawBatches.Contains(mesh.meshAssetID))
        {
	        MeshDrawBatch batch;
            batch.programHandle = m_program;
            batch.vertexBufferHandle = m_meshData[mesh.meshAssetID].vertexBufferHandle;
            batch.indexBufferHandle = m_meshData[mesh.meshAssetID].indexBufferHandle;

            m_meshDrawBatches.Insert(mesh.meshAssetID, batch);
        }

        m_meshDrawBatches[mesh.meshAssetID].entities.insert(entity->ID());
	}

	void BGFXRenderSystem::CleanupMeshComponent(std::shared_ptr<ECS::Entity> entity)
	{
        auto& mesh = entity->GetComponent<MeshComponent>();

        m_meshDrawBatches[mesh.meshAssetID].entities.erase(entity->ID());

        if (m_meshDrawBatches[mesh.meshAssetID].entities.empty())
        {
	        m_meshDrawBatches.Erase(mesh.meshAssetID);
        }

        m_meshData[mesh.meshAssetID].entities.erase(entity->ID());

        if (m_meshData[mesh.meshAssetID].entities.empty())
        {
            bgfx::destroy(m_meshData[mesh.meshAssetID].vertexBufferHandle);
            bgfx::destroy(m_meshData[mesh.meshAssetID].indexBufferHandle);

            m_meshData.Erase(mesh.meshAssetID);
        }
	}

	void BGFXRenderSystem::LoadAndInitMesh(UUID meshID)
	{
        const auto staticMeshAsset = std::static_pointer_cast<Assets::StaticMeshAsset>(Assets::AssetRegistry::Get()->GetAsset(meshID));

        if (staticMeshAsset && staticMeshAsset->Load())
        {
	        MeshData data;
            data.assetID = meshID;
            data.numVertices = staticMeshAsset->GetVertices().size();
            data.numIndices = staticMeshAsset->GetIndices().size();

            data.vertexBufferHandle = InitVertexBuffer(staticMeshAsset->GetVertices().data(), data.numVertices, s_layoutVertexPNTV32);
            data.indexBufferHandle = InitIndexBuffer(staticMeshAsset->GetIndices().data(), data.numIndices, true);

            m_meshData.Insert(meshID, data);
        }
	}

	bgfx::VertexBufferHandle BGFXRenderSystem::InitVertexBuffer(const void* vertices, const uint32_t& numVertices,
		const bgfx::VertexLayout& layout)
	{
		// Allocate memory for vertex buffer
        const bgfx::Memory* mem = bgfx::alloc(numVertices * layout.getStride());

        // Copy vertices to allocated memory
        bx::memCopy(mem->data, vertices, mem->size);

        // Create vertex buffer
        return bgfx::createVertexBuffer(mem, layout);
	}

	bgfx::IndexBufferHandle BGFXRenderSystem::InitIndexBuffer(const void* indices, const uint32_t numIndices,
		bool use32BitIndices)
	{
        // Set indices size and flags based on whether 16 or 32 bit indices are used
		uint16_t indicesSize;
        uint16_t indexFlags = 0;

        if (use32BitIndices)
        {
	        indicesSize = sizeof(uint32_t);
            indexFlags = BGFX_BUFFER_INDEX32;
        }
        else
        {
            indicesSize = sizeof(uint16_t);
        }

        // Allocate memory for index buffer
        const bgfx::Memory* mem = bgfx::alloc(numIndices * indicesSize);

        // Copy indices to allocated memory
        bx::memCopy(mem->data, indices, mem->size);

        // Create index buffer
        return bgfx::createIndexBuffer(mem, indexFlags);
	}
}
