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
#include "MathHelpers.h"
#include "Engine/SignalSubsystem.hpp"

#include <vector>

#include "Input/InputSubsystem.h"

namespace Puffin::Rendering::BGFX
{
	void BGFXRenderSystem::Init()
	{
        bgfx::PlatformData pd;
        GLFWwindow* window = m_engine->GetSubsystem<Window::WindowSubsystem>()->GetPrimaryWindow();

        glfwSetWindowUserPointer(window, this);
        glfwSetFramebufferSizeCallback(window, FrameBufferResizeCallback);

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

        glfwGetWindowSize(window, &m_windowWidth, &m_windowHeight);

        bgfxInit.resolution.width = static_cast<uint32_t>(m_windowWidth);
        bgfxInit.resolution.height = static_cast<uint32_t>(m_windowHeight);
        bgfxInit.resolution.reset = BGFX_RESET_VSYNC;
        bgfxInit.platformData = pd;
        bgfx::init(bgfxInit);

        bgfx::setDebug(BGFX_DEBUG_NONE);

        bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x00B5E2FF, 1.0f, 0);
        bgfx::setViewRect(0, 0, 0, static_cast<uint16_t>(m_windowWidth), static_cast<uint16_t>(m_windowHeight));

        InitStaticCubeData();

        InitMeshProgram();

        // Connect Signls
        auto signalSubsystem = m_engine->GetSubsystem<Core::SignalSubsystem>();

        signalSubsystem->Connect<Input::InputEvent>(
			[&](const Input::InputEvent& inputEvent)
			{
				shared_from_this()->OnInputEvent(inputEvent);
			}
        );

        // Register Components
        m_world->RegisterComponent<CameraMatComponent>();
        m_world->AddComponentDependencies<CameraComponent, CameraMatComponent>();
	}

	void BGFXRenderSystem::PreStart()
	{
		InitEditorCamera();

        InitComponents();
	}

	void BGFXRenderSystem::Update()
	{
		ProcessEvents();

        UpdateEditorCamera();

		UpdateComponents();

        Draw();
	}

	void BGFXRenderSystem::Stop()
	{
		CleanupComponents();
	}

	void BGFXRenderSystem::Cleanup()
	{
        bgfx::destroy(m_meshProgram);

        DestroyStaticCubeData();

		bgfx::shutdown();
	}

	void BGFXRenderSystem::OnInputEvent(const Input::InputEvent& inputEvent)
	{
        m_inputEvents.Push(inputEvent);
	}

	void BGFXRenderSystem::InitStaticCubeData()
	{
        // Create Static Vertex Buffer
        m_cubeMeshData.vertexBufferHandle = bgfx::createVertexBuffer(bgfx::makeRef(s_cubeVertices, sizeof(s_cubeVertices)), s_layoutVertexPC32);

        // Create Static Index Buffer
        m_cubeMeshData.indexBufferHandle = bgfx::createIndexBuffer(bgfx::makeRef(s_cubeTriList, sizeof(s_cubeTriList)));

        bgfx::ShaderHandle cubeVSH = LoadShader("C:\\Projects\\PuffinEngine\\bin\\spirv\\vs_cubes.bin");
        bgfx::ShaderHandle cubeFSH = LoadShader("C:\\Projects\\PuffinEngine\\bin\\spirv\\fs_cubes.bin");
        m_cubeProgram = bgfx::createProgram(cubeVSH, cubeFSH, true);
	}

	void BGFXRenderSystem::DestroyStaticCubeData()
	{
        bgfx::destroy(m_cubeMeshData.indexBufferHandle);
        bgfx::destroy(m_cubeMeshData.vertexBufferHandle);

        bgfx::destroy(m_cubeProgram);
	}

	void BGFXRenderSystem::InitMeshProgram()
	{
        bgfx::ShaderHandle meshVSH = LoadShader("C:\\Projects\\PuffinEngine\\bin\\spirv\\forward_shading\\vs_forward_shading.bin");
        bgfx::ShaderHandle meshFSH = LoadShader("C:\\Projects\\PuffinEngine\\bin\\spirv\\forward_shading\\fs_forward_shading.bin");

        m_meshProgram = bgfx::createProgram(meshVSH, meshFSH, true);
	}

	void BGFXRenderSystem::ProcessEvents()
	{
        Input::InputEvent inputEvent;
		while(m_inputEvents.Pop(inputEvent))
		{
            if (inputEvent.actionName == "CamMoveLeft")
            {
                if (inputEvent.actionState == Puffin::Input::KeyState::PRESSED)
                {
                    m_moveLeft = true;
                }
                else if (inputEvent.actionState == Puffin::Input::KeyState::RELEASED)
                {
                    m_moveLeft = false;
                }
            }

            if (inputEvent.actionName == "CamMoveRight")
            {
                if (inputEvent.actionState == Puffin::Input::KeyState::PRESSED)
                {
                    m_moveRight = true;
                }
                else if (inputEvent.actionState == Puffin::Input::KeyState::RELEASED)
                {
                    m_moveRight = false;
                }
            }

            if (inputEvent.actionName == "CamMoveForward")
            {
                if (inputEvent.actionState == Puffin::Input::KeyState::PRESSED)
                {
                    m_moveForward = true;
                }
                else if (inputEvent.actionState == Puffin::Input::KeyState::RELEASED)
                {
                    m_moveForward = false;
                }
            }

            if (inputEvent.actionName == "CamMoveBackward")
            {
                if (inputEvent.actionState == Puffin::Input::KeyState::PRESSED)
                {
                    m_moveBackward = true;
                }
                else if (inputEvent.actionState == Puffin::Input::KeyState::RELEASED)
                {
                    m_moveBackward = false;
                }
            }

            if (inputEvent.actionName == "CamMoveUp")
            {
                if (inputEvent.actionState == Puffin::Input::KeyState::PRESSED)
                {
                    m_moveUp = true;
                }
                else if (inputEvent.actionState == Puffin::Input::KeyState::RELEASED)
                {
                    m_moveUp = false;
                }
            }

            if (inputEvent.actionName == "CamMoveDown")
            {
                if (inputEvent.actionState == Puffin::Input::KeyState::PRESSED)
                {
                    m_moveDown = true;
                }
                else if (inputEvent.actionState == Puffin::Input::KeyState::RELEASED)
                {
                    m_moveDown = false;
                }
            }
		}
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

        std::vector<std::shared_ptr<ECS::Entity>> cameraEntities;
        ECS::GetEntities<TransformComponent, CameraComponent>(m_world, cameraEntities);
        for (const auto& entity : cameraEntities)
        {
            if (entity->GetComponentFlag<CameraComponent, FlagDirty>())
            {
				UpdateCameraComponent(entity);

                entity->SetComponentFlag<CameraComponent, FlagDirty>(false);
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

        std::vector<std::shared_ptr<ECS::Entity>> cameraEntities;
        ECS::GetEntities<TransformComponent, CameraComponent>(m_world, cameraEntities);
        for (const auto& entity : cameraEntities)
        {
            if (entity->GetComponentFlag<CameraComponent, FlagDirty>())
            {
                UpdateCameraComponent(entity);

                entity->SetComponentFlag<CameraComponent, FlagDirty>(false);
            }

            if (entity->GetComponentFlag<CameraComponent, FlagDeleted>())
            {
                entity->RemoveComponent<CameraComponent>();
                entity->RemoveComponent<CameraMatComponent>();
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
        // Resize view and back-buffer
        if (m_windowResized)
        {
            bgfx::setViewRect(0, 0, 0, static_cast<uint16_t>(m_windowWidth), static_cast<uint16_t>(m_windowHeight));
            bgfx::reset(static_cast<uint16_t>(m_windowWidth), static_cast<uint16_t>(m_windowHeight));

            m_windowResized = false;
        }

        // Dummy draw call to make sure view 0 is cleared
        bgfx::touch(0);

        // Set Camera Matrices
        bgfx::setViewTransform(0, m_editorCamMats.view, m_editorCamMats.proj);

        for (const auto& drawBatch : m_meshDrawBatches)
        {
            // Set Vertex/Index Buffers
	        bgfx::setVertexBuffer(0, drawBatch.vertexBufferHandle);
	        bgfx::setIndexBuffer(drawBatch.indexBufferHandle);

            for (const auto& entity : drawBatch.entities)
            {
				const auto& transform = m_world->GetComponent<TransformComponent>(entity);

                // Setup Transform
				float model[16];

                BuildModelTransform(transform, model);

                bgfx::setTransform(model);

                // Submit Program
				bgfx::submit(0, drawBatch.programHandle);
            }
        }

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
            batch.programHandle = m_meshProgram;
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

	void BGFXRenderSystem::InitEditorCamera()
	{
        m_editorCam.position = {0.0f, 0.0f, 15.0f};
        m_editorCam.aspect = static_cast<float>(m_windowWidth) / static_cast<float>(m_windowHeight);
        m_editorCam.lookat = m_editorCam.position + m_editorCam.direction;

        bx::mtxLookAt(m_editorCamMats.view, static_cast<bx::Vec3>(m_editorCam.position), static_cast<bx::Vec3>(m_editorCam.lookat), { 0, 1, 0 }, bx::Handedness::Right);
        bx::mtxProj(m_editorCamMats.proj, m_editorCam.fovY, m_editorCam.aspect, m_editorCam.zNear, m_editorCam.zFar, bgfx::getCaps()->homogeneousDepth, bx::Handedness::Right);
	}

	void BGFXRenderSystem::UpdateEditorCamera()
    {
        const auto inputSubsystem = m_engine->GetSubsystem<Input::InputSubsystem>();

        if (inputSubsystem->IsCursorLocked())
        {
            // Camera Movement
            if (m_moveLeft && !m_moveRight)
            {
                m_editorCam.position += m_editorCam.right * m_editorCam.speed * m_engine->GetDeltaTime();
            }

            if (m_moveRight && !m_moveLeft)
            {
                m_editorCam.position -= m_editorCam.right * m_editorCam.speed * m_engine->GetDeltaTime();
            }

            if (m_moveForward && !m_moveBackward)
            {
                m_editorCam.position += m_editorCam.direction * m_editorCam.speed * m_engine->GetDeltaTime();
            }

            if (m_moveBackward && !m_moveForward)
            {
                m_editorCam.position -= m_editorCam.direction * m_editorCam.speed * m_engine->GetDeltaTime();
            }

            if (m_moveUp && !m_moveDown)
            {
                m_editorCam.position += m_editorCam.up * m_editorCam.speed * m_engine->GetDeltaTime();
            }

            if (m_moveDown && !m_moveUp)
            {
                m_editorCam.position -= m_editorCam.up * m_editorCam.speed * m_engine->GetDeltaTime();
            }

            // Mouse Rotation
            m_editorCam.yaw += inputSubsystem->GetMouseXOffset();
            m_editorCam.pitch -= inputSubsystem->GetMouseYOffset();

            if (m_editorCam.pitch > 89.0f)
                m_editorCam.pitch = 89.0f;

            if (m_editorCam.pitch < -89.0f)
                m_editorCam.pitch = -89.0f;

            // Calculate Direction vector from yaw and pitch of camera
            m_editorCam.direction.x = cos(Maths::DegreesToRadians(m_editorCam.yaw)) * cos(Maths::DegreesToRadians(m_editorCam.pitch));
            m_editorCam.direction.y = sin(Maths::DegreesToRadians(m_editorCam.pitch));
            m_editorCam.direction.z = sin(Maths::DegreesToRadians(m_editorCam.yaw)) * cos(Maths::DegreesToRadians(m_editorCam.pitch));

            m_editorCam.direction.Normalise();
        }

        // Calculate Right, Up and LookAt vectors
        m_editorCam.right = m_editorCam.up.Cross(m_editorCam.direction).Normalised();
        m_editorCam.lookat = m_editorCam.position + m_editorCam.direction;

        if (m_windowResized)
        {
            m_editorCam.aspect = static_cast<float>(m_windowWidth) / static_cast<float>(m_windowHeight);
        }

        bx::mtxLookAt(m_editorCamMats.view, static_cast<bx::Vec3>(m_editorCam.position), static_cast<bx::Vec3>(m_editorCam.lookat), {0, 1, 0}, bx::Handedness::Right);
        bx::mtxProj(m_editorCamMats.proj, m_editorCam.fovY, m_editorCam.aspect, m_editorCam.zNear, m_editorCam.zFar, bgfx::getCaps()->homogeneousDepth, bx::Handedness::Right);
    }

	void BGFXRenderSystem::UpdateCameraComponent(std::shared_ptr<ECS::Entity> entity)
	{
        auto& transform = entity->GetComponent<TransformComponent>();
        auto& cam = entity->GetComponent<CameraComponent>();
        auto& camMats = entity->GetComponent<CameraMatComponent>();

        // Calculate Right, Up and LookAt vectors
        cam.right = cam.up.Cross(cam.direction).Normalised();
        cam.lookat = transform.position + cam.direction;

        bx::mtxLookAt(camMats.view, static_cast<bx::Vec3>(transform.position), static_cast<bx::Vec3>(cam.lookat), { 0, 1, 0 }, bx::Handedness::Right);

        // Recalculate camera perspective if fov has changed, store new fov in prevFov
        if (cam.fovY != cam.prevFovY)
        {
            cam.prevFovY = cam.fovY;

            bx::mtxProj(camMats.proj, cam.fovY, cam.aspect, cam.zNear, cam.zFar, bgfx::getCaps()->homogeneousDepth, bx::Handedness::Right);
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

	void BGFXRenderSystem::BuildModelTransform(const TransformComponent& transform, float* model)
	{
        // Identity
		bx::mtxIdentity(model);

        // Scale
        bx::mtxScale(model, transform.scale.x, transform.scale.y, transform.scale.z);

        // Rotation 
        bx::mtxRotateX(model, Maths::DegreesToRadians(transform.rotation.x));
        bx::mtxRotateY(model, Maths::DegreesToRadians(transform.rotation.y));
        bx::mtxRotateZ(model, Maths::DegreesToRadians(transform.rotation.z));

        // Translation
        bx::mtxTranslate(model, static_cast<float>(transform.position.x), static_cast<float>(transform.position.y), static_cast<float>(transform.position.z));
	}
}
