#include "Rendering/BGFX/BGFXRenderSystem.hpp"

#include "bgfx/bgfx.h"
#include "bx/math.h"

#include "Engine/Engine.hpp"
#include "Window/WindowSubsystem.hpp"

#include "Assets/AssetRegistry.h"
#include "GLFW/glfw3.h"
#include "GLFW/glfw3native.h"

#include "Components/TransformComponent.h"
#include "Components/Rendering/MeshComponent.h"
#include "Components/Rendering/CameraComponent.h"
#include "Components/Rendering/LightComponent.h"
#include "MathHelpers.h"
#include "Engine/SignalSubsystem.hpp"
#include "Assets/TextureAsset.h"
#include "Input/InputSubsystem.h"

#include <vector>

namespace Puffin::Rendering::BGFX
{
	void BGFXRenderSystem::Init()
	{
        InitBGFX();

        InitStaticCubeData();

        InitMeshProgram();

        InitTexSamplers();

        InitCamUniforms();

        InitLightUniforms();

        // Connect Signals
        const auto signalSubsystem = m_engine->GetSubsystem<Core::SignalSubsystem>();

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
        m_deletionQueue.Flush();

		bgfx::shutdown();
	}

	void BGFXRenderSystem::OnInputEvent(const Input::InputEvent& inputEvent)
	{
        m_inputEvents.Push(inputEvent);
	}

	void BGFXRenderSystem::InitBGFX()
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

        // Check supported features
        const bgfx::Caps* caps = bgfx::getCaps();

        m_supportsInstancing = 0 != (BGFX_CAPS_INSTANCING & caps->supported);
	}

	void BGFXRenderSystem::InitStaticCubeData()
	{
        // Create Static Vertex Buffer
        m_cubeMeshData.vertexBufferHandle = bgfx::createVertexBuffer(bgfx::makeRef(s_cubeVertices, sizeof(s_cubeVertices)), s_layoutVertexPC32);

        // Create Static Index Buffer
        m_cubeMeshData.indexBufferHandle = bgfx::createIndexBuffer(bgfx::makeRef(s_cubeTriList, sizeof(s_cubeTriList)));

        bgfx::ShaderHandle cubeVSH = LoadShader("C:\\Projects\\PuffinEngine\\bin\\bgfx\\spirv\\vs_cubes.bin");
        bgfx::ShaderHandle cubeFSH = LoadShader("C:\\Projects\\PuffinEngine\\bin\\bgfx\\spirv\\fs_cubes.bin");
        m_cubeProgram = bgfx::createProgram(cubeVSH, cubeFSH, true);

        m_deletionQueue.PushFunction([=]()
        {
        	bgfx::destroy(m_cubeMeshData.indexBufferHandle);
			bgfx::destroy(m_cubeMeshData.vertexBufferHandle);

			bgfx::destroy(m_cubeProgram);
        });
	}

	void BGFXRenderSystem::InitMeshProgram()
	{
        bgfx::ShaderHandle meshVSH = LoadShader("C:\\Projects\\PuffinEngine\\bin\\bgfx\\spirv\\forward_shading\\vs_forward_shading.bin");
        bgfx::ShaderHandle meshFSH = LoadShader("C:\\Projects\\PuffinEngine\\bin\\bgfx\\spirv\\forward_shading\\fs_forward_shading.bin");

        m_meshProgram = bgfx::createProgram(meshVSH, meshFSH, true);

        m_deletionQueue.PushFunction([=]()
        {
        	bgfx::destroy(m_meshProgram);
        });
	}

	void BGFXRenderSystem::InitMeshInstancedProgram()
	{
        bgfx::ShaderHandle meshVSH = LoadShader("C:\\Projects\\PuffinEngine\\bin\\bgfx\\spirv\\forward_shading\\vs_forward_shading_instanced.bin");
        bgfx::ShaderHandle meshFSH = LoadShader("C:\\Projects\\PuffinEngine\\bin\\bgfx\\spirv\\forward_shading\\fs_forward_shading_instanced.bin");

        m_meshInstancedProgram = bgfx::createProgram(meshVSH, meshFSH, true);

        m_deletionQueue.PushFunction([=]()
            {
                bgfx::destroy(m_meshInstancedProgram);
            });
	}

	void BGFXRenderSystem::InitTexSamplers()
	{
        m_texAlbedoSampler = bgfx::createUniform("s_texColor", bgfx::UniformType::Sampler);
        m_texNormalSampler = bgfx::createUniform("s_texNormal", bgfx::UniformType::Sampler);

        m_texAlbedoArray.Init(4096, 4096);

        m_deletionQueue.PushFunction([=]()
        {
        	bgfx::destroy(m_texAlbedoSampler);
			bgfx::destroy(m_texNormalSampler);
        });
	}

	void BGFXRenderSystem::InitCamUniforms()
	{
		m_camPosHandle = bgfx::createUniform("u_camPos", bgfx::UniformType::Vec4, 1);

        m_deletionQueue.PushFunction([=]()
        {
        	bgfx::destroy(m_camPosHandle);
        });
	}

	void BGFXRenderSystem::InitLightUniforms()
	{
        m_lightUniformHandles.position = bgfx::createUniform("u_lightPos", bgfx::UniformType::Vec4, G_MAX_LIGHTS);
        m_lightUniformHandles.direction = bgfx::createUniform("u_lightDir", bgfx::UniformType::Vec4, G_MAX_LIGHTS);
        m_lightUniformHandles.color = bgfx::createUniform("u_lightColor", bgfx::UniformType::Vec4, G_MAX_LIGHTS);
        m_lightUniformHandles.ambientSpecular = bgfx::createUniform("u_lightAmbientSpecular", bgfx::UniformType::Vec4, G_MAX_LIGHTS);
        m_lightUniformHandles.attenuation = bgfx::createUniform("u_lightAttenuation", bgfx::UniformType::Vec4, G_MAX_LIGHTS);

        m_lightUniformHandles.index = bgfx::createUniform("u_lightIndex", bgfx::UniformType::Vec4, 1);

        m_deletionQueue.PushFunction([=]()
        {
            bgfx::destroy(m_lightUniformHandles.position);
	        bgfx::destroy(m_lightUniformHandles.direction);
	        bgfx::destroy(m_lightUniformHandles.color);
	        bgfx::destroy(m_lightUniformHandles.ambientSpecular);
            bgfx::destroy(m_lightUniformHandles.attenuation);
            bgfx::destroy(m_lightUniformHandles.index);
        });
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

        std::vector<std::shared_ptr<ECS::Entity>> lightEntities;
        ECS::GetEntities<TransformComponent, LightComponent>(m_world, lightEntities);
        for (const auto& entity : lightEntities)
        {
	        
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

        if (m_useInstancing && m_supportsInstancing)
        {
            DrawSceneInstanced();
        }
        else
        {
            DrawScene();
        }

        // Advance to next frame
        bgfx::frame();

        m_frameCounter++;
	}

	void BGFXRenderSystem::DrawScene()
	{
        // Set Camera Matrices
        bgfx::setViewTransform(0, m_editorCamMats.view, m_editorCamMats.proj);

        // Set Light Uniforms
        SetupLightUniformsForDraw();

        for (const auto& drawBatch : m_meshDrawBatches)
        {
            // Set Vertex/Index Buffers
            bgfx::setVertexBuffer(0, drawBatch.vertexBufferHandle);
            bgfx::setIndexBuffer(drawBatch.indexBufferHandle);

            for (const auto& entity : drawBatch.entities)
            {
                const auto& transform = m_world->GetComponent<TransformComponent>(entity);
                const auto& mesh = m_world->GetComponent<MeshComponent>(entity);

                // Setup Transform
                float model[16];

                BuildModelTransform(transform, model);

                bgfx::setTransform(model);

                // Set Texture
                bgfx::setTexture(0, m_texAlbedoSampler, m_texAlbedoHandles[mesh.textureAssetID].handle);

                // Submit Program
                bgfx::submit(0, drawBatch.programHandle, 0, BGFX_DISCARD_TRANSFORM | BGFX_DISCARD_STATE | BGFX_DISCARD_BINDINGS);
            }

            // Discard Vertex/Index Buffers after rendering all entities in batch
            bgfx::discard(BGFX_DISCARD_VERTEX_STREAMS | BGFX_DISCARD_INDEX_BUFFER);
        }
	}

	void BGFXRenderSystem::DrawSceneInstanced()
	{
        // Set Camera Matrices
        bgfx::setViewTransform(0, m_editorCamMats.view, m_editorCamMats.proj);

        // Set Light Uniforms
        SetupLightUniformsForDraw();

        // Setup stride for instanced rendering
        const uint16_t instanceStride = 80; // 64 Bytes for 4x4 matrix, 16 Bytes for Texture Index

        // Set Texture Arrays
        bgfx::setTexture(0, m_texAlbedoSampler, m_texAlbedoArray.Handle());

        for (const auto& drawBatch : m_meshDrawBatches)
        {
            // Set Vertex/Index Buffers
            bgfx::setVertexBuffer(0, drawBatch.vertexBufferHandle);
            bgfx::setIndexBuffer(drawBatch.indexBufferHandle);

            const uint32_t numEntities = drawBatch.entities.size();
            const uint32_t numInstances = bgfx::getAvailInstanceDataBuffer(numEntities, instanceStride);

            bgfx::InstanceDataBuffer idb = {};
            bgfx::allocInstanceDataBuffer(&idb, numInstances, instanceStride);

            uint8_t* data = idb.data;

            for (const auto& entity : drawBatch.entities)
            {
                const auto& transform = m_world->GetComponent<TransformComponent>(entity);
                const auto& mesh = m_world->GetComponent<MeshComponent>(entity);

                // Setup Transform
                float* mtx = (float*)data;

                BuildModelTransform(transform, mtx);

                float* index = (float*)&data[64];
                index[0] = static_cast<float>(m_texAlbedoArray.GetTextureIndex(mesh.textureAssetID));
                index[1] = 0.0f;
                index[2] = 0.0f;
                index[3] = 0.0f;

                data += instanceStride;
            }

            bgfx::setInstanceDataBuffer(&idb, 0, numInstances);

            bgfx::setState(BGFX_STATE_DEFAULT);

            // Submit Program
            bgfx::submit(0, drawBatch.programHandle, 0, BGFX_DISCARD_VERTEX_STREAMS | BGFX_DISCARD_INDEX_BUFFER);
        }

        // Discard Texture/Buffer bindings
        bgfx::discard(BGFX_DISCARD_TRANSFORM | BGFX_DISCARD_BINDINGS | BGFX_DISCARD_STATE);
	}

	void BGFXRenderSystem::InitMeshComponent(std::shared_ptr<ECS::Entity> entity)
	{
        auto& mesh = entity->GetComponent<MeshComponent>();

        // Init Mesh
        if (!m_meshData.Contains(mesh.meshAssetID))
        {
	        LoadAndInitMesh(mesh.meshAssetID);

            m_meshData[mesh.meshAssetID].entities.insert(entity->ID());
        }

        if (!m_meshDrawBatches.Contains(mesh.meshAssetID))
        {
	        MeshDrawBatch batch;

            // Set shader program depending on if instancing is supported
            if (m_useInstancing && m_supportsInstancing)
            {
                batch.programHandle = m_meshInstancedProgram;
            }
            else
            {
                batch.programHandle = m_meshProgram;
            }
            
            batch.vertexBufferHandle = m_meshData[mesh.meshAssetID].vertexBufferHandle;
            batch.indexBufferHandle = m_meshData[mesh.meshAssetID].indexBufferHandle;

            m_meshDrawBatches.Insert(mesh.meshAssetID, batch);
        }

        m_meshDrawBatches[mesh.meshAssetID].entities.insert(entity->ID());

        // Init Textures
        if (!m_texAlbedoHandles.Contains(mesh.textureAssetID))
        {
            TextureData texData;
            texData.handle = LoadAndInitTexture(mesh.textureAssetID);

            m_texAlbedoHandles.Emplace(mesh.textureAssetID, texData);
        }

        m_texAlbedoHandles[mesh.textureAssetID].entities.insert(entity->ID());

        if (m_useInstancing && m_supportsInstancing)
        {
			m_texAlbedoArray.AddTexture(mesh.textureAssetID);
        }
	}

	void BGFXRenderSystem::CleanupMeshComponent(std::shared_ptr<ECS::Entity> entity)
	{
        auto& mesh = entity->GetComponent<MeshComponent>();

        // Cleanup Mesh Data
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

        // Cleanup Texture Data
        m_texAlbedoHandles[mesh.textureAssetID].entities.erase(entity->ID());

        if (m_texAlbedoHandles[mesh.textureAssetID].entities.empty())
        {
			bgfx::destroy(m_texAlbedoHandles[mesh.textureAssetID].handle);

            m_texAlbedoHandles.Erase(mesh.textureAssetID);
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
        const auto meshAsset = std::static_pointer_cast<Assets::StaticMeshAsset>(Assets::AssetRegistry::Get()->GetAsset(meshID));

        if (meshAsset && meshAsset->Load())
        {
	        MeshData data;
            data.assetID = meshID;
            data.numVertices = meshAsset->GetVertices().size();
            data.numIndices = meshAsset->GetIndices().size();

            data.vertexBufferHandle = InitVertexBuffer(meshAsset->GetVertices().data(), data.numVertices, s_layoutVertexPNTV32);
            data.indexBufferHandle = InitIndexBuffer(meshAsset->GetIndices().data(), data.numIndices, true);

            m_meshData.Insert(meshID, data);

            meshAsset->Unload();
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

    bgfx::TextureHandle BGFXRenderSystem::LoadAndInitTexture(UUID texID) const
    {
        const auto texAsset = std::static_pointer_cast<Assets::TextureAsset>(Assets::AssetRegistry::Get()->GetAsset(texID));

        bgfx::TextureHandle handle = BGFX_INVALID_HANDLE;

        if (texAsset && texAsset->Load())
        {
			uint32_t texSize = texAsset->GetTextureSize();

	        const bgfx::Memory* mem = bgfx::alloc(texSize);

            bx::memCopy(mem->data, texAsset->GetPixelData(), mem->size);

            handle = bgfx::createTexture2D(texAsset->GetTextureWidth(), texAsset->GetTextureHeight(),
            false, 1, g_texFormatMap.at(texAsset->GetTextureFormat()), 0, mem);

            texAsset->Unload();
        }

        return handle;
    }

    void BGFXRenderSystem::SetupLightUniformsForDraw() const
    {
		// Setup Light Data
        float lightPos[G_MAX_LIGHTS][4];
        float lightDir[G_MAX_LIGHTS][4];
        float lightColor[G_MAX_LIGHTS][4];
        float lightAmbientSpec[G_MAX_LIGHTS][4];
        float lightAttenuation[G_MAX_LIGHTS][4];
        float lightIndex[4];

        int index = 0;

        std::vector<std::shared_ptr<ECS::Entity>> lightEntities, lightEntitiesOrdered;
        ECS::GetEntities<TransformComponent, LightComponent>(m_world, lightEntities);

        // Sort Lights into DIRECTIONAL, POINT, SPOT Order
        lightEntitiesOrdered.reserve(lightEntities.size());
        
        for (const auto& entity : lightEntities)
        {
            // Break out of loop when number of lights exceeds max
            if (index >= G_MAX_LIGHTS)
            {
                break;
            }

            const auto& light = entity->GetComponent<LightComponent>();

	        if (light.type == LightType::DIRECTIONAL)
	        {
                lightEntitiesOrdered.push_back(entity);
                index++;
	        }
        }

        lightIndex[0] = static_cast<float>(index);

        for (const auto& entity : lightEntities)
        {
            // Break out of loop when number of lights exceeds max
            if (index >= G_MAX_LIGHTS)
            {
                break;
            }

            const auto& light = entity->GetComponent<LightComponent>();

            if (light.type == LightType::POINT)
            {
                lightEntitiesOrdered.push_back(entity);
                index++;
            }
        }

        lightIndex[1] = static_cast<float>(index);

        for (const auto& entity : lightEntities)
        {
            // Break out of loop when number of lights exceeds max
            if (index >= G_MAX_LIGHTS)
            {
                break;
            }

            const auto& light = entity->GetComponent<LightComponent>();

            if (light.type == LightType::SPOT)
            {
                lightEntitiesOrdered.push_back(entity);
                index++;
            }
        }

        lightIndex[2] = static_cast<float>(index);

        index = 0;

        // Iterate through sorted 
        for (const auto& entity : lightEntitiesOrdered)
        {
            // Break out of loop when number of lights exceeds max
			if (index >= G_MAX_LIGHTS)
			{
				break;
			}

            const auto& transform = entity->GetComponent<TransformComponent>();
            const auto& light = entity->GetComponent<LightComponent>();

            lightPos[index][0] = transform.position.x;
            lightPos[index][1] = transform.position.y;
            lightPos[index][2] = transform.position.z;
            lightPos[index][3] = 0.0f;

            lightDir[index][0] = light.direction.x;
            lightDir[index][1] = light.direction.y;
            lightDir[index][2] = light.direction.z;
            lightDir[index][3] = 0.0f;

            lightColor[index][0] = light.color.x;
            lightColor[index][1] = light.color.y;
            lightColor[index][2] = light.color.z;
            lightColor[index][3] = 0.0f;

            lightAmbientSpec[index][0] = light.ambientIntensity;
            lightAmbientSpec[index][1] = light.specularIntensity;
            lightAmbientSpec[index][2] = static_cast<float>(light.specularExponent);
            lightAmbientSpec[index][3] = 0.0f;

            lightAttenuation[index][0] = light.constantAttenuation;
            lightAttenuation[index][1] = light.linearAttenuation;
            lightAttenuation[index][2] = light.quadraticAttenuation;
            lightAttenuation[index][3] = 0.0f;

            index++;
        }

        // Set Light Uniforms
        
		bgfx::setUniform(m_lightUniformHandles.position, lightPos, G_MAX_LIGHTS);
        bgfx::setUniform(m_lightUniformHandles.direction, lightDir, G_MAX_LIGHTS);
        bgfx::setUniform(m_lightUniformHandles.color, lightColor, G_MAX_LIGHTS);
        bgfx::setUniform(m_lightUniformHandles.ambientSpecular, lightAmbientSpec, G_MAX_LIGHTS);
        bgfx::setUniform(m_lightUniformHandles.attenuation, lightAttenuation, G_MAX_LIGHTS);
        bgfx::setUniform(m_lightUniformHandles.index, lightIndex, 1);
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
