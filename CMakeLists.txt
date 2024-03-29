cmake_minimum_required(VERSION 3.22.1)

# Download CPM.cmake
set(CPM_DOWNLOAD_VERSION 0.36.0)

if(CPM_SOURCE_CACHE)
  set(CPM_DOWNLOAD_LOCATION "${CPM_SOURCE_CACHE}/cpm/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
elseif(DEFINED ENV{CPM_SOURCE_CACHE})
  set(CPM_DOWNLOAD_LOCATION "$ENV{CPM_SOURCE_CACHE}/cpm/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
else()
  set(CPM_DOWNLOAD_LOCATION "${CMAKE_BINARY_DIR}/cmake/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
endif()

if(NOT (EXISTS ${CPM_DOWNLOAD_LOCATION}))
  message(STATUS "Downloading CPM.cmake to ${CPM_DOWNLOAD_LOCATION}")
  file(DOWNLOAD
       https://github.com/TheLartians/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake
       ${CPM_DOWNLOAD_LOCATION}
  )
endif()

include(${CPM_DOWNLOAD_LOCATION})

include(FetchContent)

# The configurations we support
set(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo")

include(${CMAKE_CURRENT_SOURCE_DIR}/PuffinEngine.cmake)

# Init/Update Submodules
add_git_submodule("deps/box2d")
add_git_submodule("deps/enkits")
add_git_submodule("deps/entt")
add_git_submodule("deps/imgui")
add_git_submodule("deps/imgui-filebrowser")
add_git_submodule("deps/implot")
add_git_submodule("deps/json")
add_git_submodule("deps/lz4")
add_git_submodule("deps/OpenSimplexNoise")
add_git_submodule("deps/tinygltf")
add_git_submodule("deps/vkb")
add_git_submodule("deps/vma-hpp")
add_git_submodule("deps/jolt")

checkout_git_branch("deps/imgui" "docking")

# Define CMAKE Variables
set(PROJECT_NAME PuffinEngine)

# Set Executable/Library/Archive Output Directories
if (WIN32)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_SOURCE_DIR}/bin/win64/debug)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_SOURCE_DIR}/bin/win64/release)
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_SOURCE_DIR}/bin/win64/debug)
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_SOURCE_DIR}/bin/win64/release)
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_SOURCE_DIR}/bin/win64/debug)
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_SOURCE_DIR}/bin/win64/release)
elseif (LINUX)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_SOURCE_DIR}/bin/linux/debug)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_SOURCE_DIR}/bin/linux/release)
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_SOURCE_DIR}/bin/linux/debug)
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_SOURCE_DIR}/bin/linux/release)
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_SOURCE_DIR}/bin/linux/debug)
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_SOURCE_DIR}/bin/linux/release)
endif()

# Options
option(BUILD_ENGINE_AS_STANDALONE "Build PuffinEngine as a standalone executable" OFF)
option(DOUBLE_PRECISION_SUPPORT "Enable Double Precision Support" OFF)
option(BOX2D_PHYSICS_SUPPORT "Compile with Box2D Physics Support" OFF)
option(JOLT_PHYSICS_SUPPORT "Compile with Jolt Physics Support" ON)
option(ONAGER2D_PHYSICS_SUPPORT "Compile with Onager 2D Physics Support" OFF)

if (BUILD_ENGINE_AS_STANDALONE)
	# Set Project Name
	project (${PROJECT_NAME} DESCRIPTION "3D ECS Game Engine" LANGUAGES CXX)
endif()

set(HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/imgui)
set(VKBOOTSTRAP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/vkb/src)
set(ANGELSCRIPT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/angelscript)
set(OPENSIMPLEX_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/OpenSimplexNoise/OpenSimplexNoise)
set(ENKITS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/enkits)

set(HEADERS 
	# Assets
	${HEADER_DIR}/Assets/Asset.h
	${HEADER_DIR}/Assets/AssetRegistry.h
	${HEADER_DIR}/Assets/MeshAsset.h
	${HEADER_DIR}/Assets/SoundAsset.h
	${HEADER_DIR}/Assets/TextureAsset.h
	${HEADER_DIR}/Assets/ShaderAsset.h
	${HEADER_DIR}/Assets/MaterialAsset.h
	${HEADER_DIR}/Assets/AssetImporters.h
	# Audio
	${HEADER_DIR}/Audio/AudioSubsystem.h
	${HEADER_DIR}/Audio/MiniAudioSubsystem.h
	# Components
	${HEADER_DIR}/Components/SceneObjectComponent.h
	${HEADER_DIR}/Components/SceneHierarchyComponent.h
	${HEADER_DIR}/Components/TransformComponent2D.h
	${HEADER_DIR}/Components/TransformComponent3D.h
	${HEADER_DIR}/Components/Rendering/CameraComponent.h
	${HEADER_DIR}/Components/Rendering/LightComponent.h
	${HEADER_DIR}/Components/Rendering/MeshComponent.h
	${HEADER_DIR}/Components/Physics/2D/VelocityComponent2D.h
	${HEADER_DIR}/Components/Physics/2D/RigidbodyComponent2D.h
	${HEADER_DIR}/Components/Physics/2D/ShapeComponents2D.h
	${HEADER_DIR}/Components/Physics/3D/VelocityComponent3D.h
	${HEADER_DIR}/Components/Physics/3D/RigidbodyComponent3D.h
	${HEADER_DIR}/Components/Physics/3D/ShapeComponents3D.h
	${HEADER_DIR}/Components/Procedural/ProceduralMeshComponent.h
	${HEADER_DIR}/Components/Scripting/AngelScriptComponent.h
	# ECS
	${HEADER_DIR}/ECS/ECSSubsystem.h
	${HEADER_DIR}/ECS/EnTTSubsystem.h
	# Core
	${HEADER_DIR}/Core/Engine.h
	${HEADER_DIR}/Core/Application.h
	${HEADER_DIR}/Core/SignalSubsystem.h
	${HEADER_DIR}/Core/EnkiTSSubsystem.h
	${HEADER_DIR}/Core/SceneSubsystem.h
	${HEADER_DIR}/Core/System.h
	# Input
	${HEADER_DIR}/Input/InputEvent.h
	${HEADER_DIR}/Input/InputSubsystem.h
	# Physics
	${HEADER_DIR}/Physics/CollisionEvent.h
	${HEADER_DIR}/Physics/PhysicsConstants.h
	${HEADER_DIR}/Physics/BodyType.h
	${HEADER_DIR}/Physics/Onager2D/PhysicsTypes2D.h
	${HEADER_DIR}/Physics/Onager2D/OnagerPhysicsSystem2D.h
	${HEADER_DIR}/Physics/Onager2D/PhysicsHelpers2D.h
	${HEADER_DIR}/Physics/Onager2D/Shapes/Shape2D.h
	${HEADER_DIR}/Physics/Onager2D/Shapes/PolygonShape2D.h
	${HEADER_DIR}/Physics/Onager2D/Shapes/CircleShape2D.h
	${HEADER_DIR}/Physics/Onager2D/Shapes/BoxShape2D.h
	${HEADER_DIR}/Physics/Onager2D/Colliders/Collider2D.h
	${HEADER_DIR}/Physics/Onager2D/Colliders/PolygonCollider2D.h
	${HEADER_DIR}/Physics/Onager2D/Colliders/BoxCollider2D.h
	${HEADER_DIR}/Physics/Onager2D/Colliders/CircleCollider2D.h
	${HEADER_DIR}/Physics/Onager2D/Broadphases/Broadphase2D.h
	${HEADER_DIR}/Physics/Onager2D/Broadphases/SweepAndPruneBroadphase.h
	${HEADER_DIR}/Physics/Onager2D/Broadphases/SpatialHashBroadphase2D.h
	${HEADER_DIR}/Physics/Box2D/Box2DPhysicsSystem.h
	${HEADER_DIR}/Physics/Box2D/Box2DContactListener.h
	${HEADER_DIR}/Physics/Jolt/JoltPhysicsSystem.h
	${HEADER_DIR}/Physics/Jolt/JoltPhysicsTypes.h
	# Procedural
	${HEADER_DIR}/Procedural/ProceduralMeshGenSystem.h
	# Rendering
	${HEADER_DIR}/Rendering/MaterialGlobals.h
	# Rendering Vulkan
	${HEADER_DIR}/Rendering/Vulkan/VKRenderSystem.h
	${HEADER_DIR}/Rendering/Vulkan/VKTypes.h
	${HEADER_DIR}/Rendering/Vulkan/VKVertex.h
	${HEADER_DIR}/Rendering/Vulkan/VKHelpers.h
	${HEADER_DIR}/Rendering/Vulkan/VKDescriptors.h
	${HEADER_DIR}/Rendering/Vulkan/VKPipeline.h
	${HEADER_DIR}/Rendering/Vulkan/VKUnifiedGeometryBuffer.h
	${HEADER_DIR}/Rendering/Vulkan/VKMaterialRegistry.h
	${HEADER_DIR}/Rendering/Vulkan/VKRenderGraph.h
	${HEADER_DIR}/Rendering/Vulkan/VKResourceManager.h
	# Scripting
	${HEADER_DIR}/Scripting/AngelScript/RegisterTypeHelpers.h
	${HEADER_DIR}/Scripting/AngelScript/AngelScriptSystem.h
	${HEADER_DIR}/Scripting/AngelScript/AngelScriptEngineInterface.h
	# Types
	${HEADER_DIR}/Types/ComponentFlags.h
	${HEADER_DIR}/Types/Matrix.h
	${HEADER_DIR}/Types/PackedArray.h
	${HEADER_DIR}/Types/Quat.h
	${HEADER_DIR}/Types/RingBuffer.h
	${HEADER_DIR}/Types/UUID.h
	${HEADER_DIR}/Types/Vector.h
	${HEADER_DIR}/Types/Vertex.h
	${HEADER_DIR}/Types/DeletionQueue.h
	# UI
	${HEADER_DIR}/UI/Editor/UISubsystem.h
	${HEADER_DIR}/UI/Editor/Windows/UIWindow.h
	${HEADER_DIR}/UI/Editor/Windows/UIContentBrowser.h
	${HEADER_DIR}/UI/Editor/Windows/UIWindowEntityProperties.h
	${HEADER_DIR}/UI/Editor/Windows/UIWindowPerformance.h
	${HEADER_DIR}/UI/Editor/Windows/UIWindowSceneHierarchy.h
	${HEADER_DIR}/UI/Editor/Windows/UIWindowSettings.h
	${HEADER_DIR}/UI/Editor/Windows/UIWindowViewport.h
	# Window
	${HEADER_DIR}/Window/WindowSubsystem.h
	# Root
	${HEADER_DIR}/ManipulationGizmo.h
	${HEADER_DIR}/MathHelpers.h
	${HEADER_DIR}/ProjectSettings.h
	)

set(SOURCES 
	# Assets
	${SOURCE_DIR}/Assets/AssetRegistry.cpp
	${SOURCE_DIR}/Assets/MeshAsset.cpp
	${SOURCE_DIR}/Assets/TextureAsset.cpp
	${SOURCE_DIR}/Assets/ShaderAsset.cpp
	${SOURCE_DIR}/Assets/MaterialAsset.cpp
	${SOURCE_DIR}/Assets/AssetImporters.cpp
	# Audio
	${SOURCE_DIR}/Audio/AudioSubsystem.cpp
	${SOURCE_DIR}/Audio/MiniAudioSubsystem.cpp
	# ECS
	${SOURCE_DIR}/ECS/ECSSubsystem.cpp
	# Engine
	${SOURCE_DIR}/Core/Engine.cpp
	${SOURCE_DIR}/Core/Application.cpp
	${SOURCE_DIR}/Core/SignalSubsystem.cpp
	${SOURCE_DIR}/Core/EnkiTSSubsystem.cpp
	# Input
	${SOURCE_DIR}/Input/InputSubsystem.cpp
	# Physics
	${SOURCE_DIR}/Physics/Onager2D/OnagerPhysicsSystem2D.cpp
	${SOURCE_DIR}/Physics/Onager2D/Shapes/CircleShape2D.cpp
	${SOURCE_DIR}/Physics/Onager2D/Shapes/BoxShape2D.cpp
	${SOURCE_DIR}/Physics/Onager2D/Shapes/CircleShape2D.cpp
	${SOURCE_DIR}/Physics/Onager2D/Colliders/BoxCollider2D.cpp
	${SOURCE_DIR}/Physics/Onager2D/Colliders/CircleCollider2D.cpp
	${SOURCE_DIR}/Physics/Onager2D/Colliders/PolygonCollider2D.cpp
	${SOURCE_DIR}/Physics/Onager2D/Broadphases/SweepAndPruneBroadphase.cpp
	${SOURCE_DIR}/Physics/Onager2D/Broadphases/SpatialHashBroadphase2D.cpp
	${SOURCE_DIR}/Physics/Box2D/Box2DContactListener.cpp
	${SOURCE_DIR}/Physics/Box2D/Box2DPhysicsSystem.cpp
	${SOURCE_DIR}/Physics/Jolt/JoltPhysicsSystem.cpp
	# Procedural
	${SOURCE_DIR}/Procedural/ProceduralMeshGenSystem.cpp
	# Rendering Vulkan
	${SOURCE_DIR}/Rendering/Vulkan/VKRenderSystem.cpp
	${SOURCE_DIR}/Rendering/Vulkan/VKHelpers.cpp
	${SOURCE_DIR}/Rendering/Vulkan/VKDescriptors.cpp
	${SOURCE_DIR}/Rendering/Vulkan/VKUnifiedGeometryBuffer.cpp
	${SOURCE_DIR}/Rendering/Vulkan/VKMaterialRegistry.cpp
	# Scripting
	${SOURCE_DIR}/Scripting/AngelScript/AngelScriptSystem.cpp
	${SOURCE_DIR}/Scripting/AngelScript/AngelScriptEngineInterface.cpp
	# UI
	${SOURCE_DIR}/UI/Editor/UISubsystem.cpp
	${SOURCE_DIR}/UI/Editor/Windows/UIWindow.cpp
	${SOURCE_DIR}/UI/Editor/Windows/UIContentBrowser.cpp
	${SOURCE_DIR}/UI/Editor/Windows/UIWindowEntityProperties.cpp
	${SOURCE_DIR}/UI/Editor/Windows/UIWindowPerformance.cpp
	${SOURCE_DIR}/UI/Editor/Windows/UIWindowSceneHierarchy.cpp
	${SOURCE_DIR}/UI/Editor/Windows/UIWindowSettings.cpp
	${SOURCE_DIR}/UI/Editor/Windows/UIWindowViewport.cpp
	# Window
	${SOURCE_DIR}/Window/WindowSubsystem.cpp
	# Root 
	${SOURCE_DIR}/ManipulationGizmo.cpp
	)
	
set (IMGUI_SOURCES
	${IMGUI_DIR}/imgui.h
	${IMGUI_DIR}/imgui.cpp
	${IMGUI_DIR}/imgui_demo.cpp
	${IMGUI_DIR}/imgui_draw.cpp
	${IMGUI_DIR}/imgui_tables.cpp
	${IMGUI_DIR}/imgui_widgets.cpp
	${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
	${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
	${IMGUI_DIR}/misc/cpp/imgui_stdlib.h
	${IMGUI_DIR}/misc/cpp/imgui_stdlib.cpp
	)
	
set (OPENSIMPLEX_SOURCES
	${OPENSIMPLEX_DIR}/OpenSimplexNoise.cpp
	)
	
set (VKBOOTSTRAP_SOURCES
	${VKBOOTSTRAP_DIR}/VkBootstrap.cpp
	)
	
set (ANGELSCRIPT_SOURCES
	${ANGELSCRIPT_DIR}/scriptarray/scriptarray.cpp
	${ANGELSCRIPT_DIR}/scriptbuilder/scriptbuilder.cpp
	${ANGELSCRIPT_DIR}/scriptstdstring/scriptstdstring.cpp
	${ANGELSCRIPT_DIR}/scriptstdstring/scriptstdstring_utils.cpp
	)
	
set (ENKITS_SOURCES
	${ENKITS_DIR}/src/TaskScheduler.h
	${ENKITS_DIR}/src/TaskScheduler.cpp
	)

# Add Main as Executable/Library
if (BUILD_ENGINE_AS_STANDALONE)
	add_executable(${PROJECT_NAME} ${HEADERS} ${SOURCES} ${SOURCE_DIR}/main.cpp )
else()
	add_library(${PROJECT_NAME} ${HEADERS} ${SOURCES})
endif()

# Set C++ Language Standard to C++ 17
set_target_properties(${PROJECT_NAME} PROPERTIES CMAKE_CXX_STANDARD 17)
set_target_properties(${PROJECT_NAME} PROPERTIES CMAKE_CXX_STANDARD_REQUIRED ON)
set_target_properties(${PROJECT_NAME} PROPERTIES CMAKE_CXX_EXTENSIONS ON)

target_sources(${PROJECT_NAME} PRIVATE ${IMGUI_SOURCES})
target_sources(${PROJECT_NAME} PRIVATE ${OPENSIMPLEX_SOURCES})
target_sources(${PROJECT_NAME} PRIVATE ${VKBOOTSTRAP_SOURCES})
target_sources(${PROJECT_NAME} PRIVATE ${ANGELSCRIPT_SOURCES})
target_sources(${PROJECT_NAME} PRIVATE ${ENKITS_SOURCES})

message("")
message("============================================================")
message("Sorting Source Files into relative Subfolders")
message("============================================================")

sort_into_source_group(HEADERS ${HEADER_DIR} "")
sort_into_source_group(SOURCES ${SOURCE_DIR} "")
sort_into_source_group(IMGUI_SOURCES ${IMGUI_DIR} Libraries/ImGui)
sort_into_source_group(OPENSIMPLEX_SOURCES ${OPENSIMPLEX_DIR} Libraries/OpenSimplexNoise)
sort_into_source_group(VKBOOTSTRAP_SOURCES ${VKBOOTSTRAP_DIR} Libraries/VKBootstrap)
sort_into_source_group(ANGELSCRIPT_SOURCES ${ANGELSCRIPT_DIR} Libraries/AngelScript)
sort_into_source_group(ENKITS_SOURCES ${ENKITS_DIR} Libraries/EnkiTS)

message("")

set(PLATFORM ${CMAKE_HOST_SYSTEM_NAME})

# Get & Build Required Libraries
find_package(Vulkan REQUIRED)
CPMAddPackage("gh:nlohmann/json@3.11.2")

target_link_libraries(${PROJECT_NAME} ${Vulkan_LIBRARIES})
target_link_libraries(${PROJECT_NAME} nlohmann_json::nlohmann_json)

if (CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
	set(LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib/win64)
	set(LIB_SUFFIX .lib)
	set(DLL_SUFFIX .dll)
elseif (CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
	set(LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib/linux)
	set(LIB_SUFFIX .a)
	set(DLL_SUFFIX .so)
endif()

# Define Debug Libs
target_link_libraries(${PROJECT_NAME} debug ${LIB_DIR}/glfw/Debug/glfw3${LIB_SUFFIX})
target_link_libraries(${PROJECT_NAME} debug ${LIB_DIR}/angelscript/Debug/angelscript64d${LIB_SUFFIX})
target_link_libraries(${PROJECT_NAME} debug ${LIB_DIR}/lz4/Debug/liblz4_static${LIB_SUFFIX})
target_link_libraries(${PROJECT_NAME} debug ${LIB_DIR}/compressonator/Compressonator_MTd${LIB_SUFFIX})

# Define Release Libs
target_link_libraries(${PROJECT_NAME} optimized ${LIB_DIR}/glfw/Release/glfw3${LIB_SUFFIX})
target_link_libraries(${PROJECT_NAME} optimized ${LIB_DIR}/angelscript/Release/angelscript64${LIB_SUFFIX})
target_link_libraries(${PROJECT_NAME} optimized ${LIB_DIR}/lz4/Release/liblz4_static${LIB_SUFFIX})
target_link_libraries(${PROJECT_NAME} optimized ${LIB_DIR}/compressonator/Compressonator_MT${LIB_SUFFIX})

if (BOX2D_PHYSICS_SUPPORT)
	target_link_libraries(${PROJECT_NAME} debug ${LIB_DIR}/box2d/Debug/box2d${LIB_SUFFIX})
	target_link_libraries(${PROJECT_NAME} optimized ${LIB_DIR}/box2d/Release/box2d${LIB_SUFFIX})
endif()

if (JOLT_PHYSICS_SUPPORT)
	if (DOUBLE_PRECISION_SUPPORT)
		target_link_libraries(${PROJECT_NAME} debug ${LIB_DIR}/jolt/Debug/Jolt_Double${LIB_SUFFIX})
		target_link_libraries(${PROJECT_NAME} optimized ${LIB_DIR}/jolt/Release/Jolt_Double${LIB_SUFFIX})
	else ()
		target_link_libraries(${PROJECT_NAME} debug ${LIB_DIR}/jolt/Debug/Jolt${LIB_SUFFIX})
		target_link_libraries(${PROJECT_NAME} optimized ${LIB_DIR}/jolt/Release/Jolt${LIB_SUFFIX})
	endif ()
endif()

set(DEPS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps)

# Add include paths
target_include_directories(${PROJECT_NAME} PUBLIC include)
target_include_directories(${PROJECT_NAME} PUBLIC third_party)
target_include_directories(${PROJECT_NAME} PUBLIC third_party/angelscript)
target_include_directories(${PROJECT_NAME} PUBLIC $ENV{VULKAN_SDK}/include)
target_include_directories(${PROJECT_NAME} PUBLIC ${DEPS_DIR}/box2d/include)
target_include_directories(${PROJECT_NAME} PUBLIC ${DEPS_DIR}/OpenSimplexNoise)
target_include_directories(${PROJECT_NAME} PUBLIC ${DEPS_DIR}/imgui)
target_include_directories(${PROJECT_NAME} PUBLIC ${DEPS_DIR}/imgui-filebrowser)
target_include_directories(${PROJECT_NAME} PUBLIC ${DEPS_DIR}/lz4/lib)
target_include_directories(${PROJECT_NAME} PUBLIC ${DEPS_DIR}/vma-hpp/VulkanMemoryAllocator/include)
target_include_directories(${PROJECT_NAME} PUBLIC ${DEPS_DIR}/vma-hpp/include)
target_include_directories(${PROJECT_NAME} PUBLIC ${DEPS_DIR}/vkb/src)
target_include_directories(${PROJECT_NAME} PUBLIC ${DEPS_DIR}/tinygltf)
target_include_directories(${PROJECT_NAME} PUBLIC ${DEPS_DIR}/enkits/src)
target_include_directories(${PROJECT_NAME} PUBLIC ${DEPS_DIR}/entt/src)
target_include_directories(${PROJECT_NAME} PUBLIC ${DEPS_DIR}/jolt)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)

set_property(TARGET ${PROJECT_NAME} PROPERTY 
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
	
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

# Add definitions for supported functionality
if (DOUBLE_PRECISION_SUPPORT)
	target_compile_definitions(${PROJECT_NAME} PUBLIC PFN_DOUBLE_PRECISION)
endif()

if (BOX2D_PHYSICS_SUPPORT)
	target_compile_definitions(${PROJECT_NAME} PUBLIC PFN_BOX2D_PHYSICS)
endif()

if (JOLT_PHYSICS_SUPPORT)
	target_compile_definitions(${PROJECT_NAME} PUBLIC PFN_JOLT_PHYSICS)
endif()

if (ONAGER2D_PHYSICS_SUPPORT)
	target_compile_definitions(${PROJECT_NAME} PUBLIC PFN_ONAGER2D_PHYSICS)
endif()

target_compile_definitions(${PROJECT_NAME} PUBLIC GLM_FORCE_DEFAULT_ALIGNED_GENTYPES)
target_compile_definitions(${PROJECT_NAME} PUBLIC GLM_FORCE_SIMD_AVX2)
target_compile_definitions(${PROJECT_NAME} PUBLIC NOMINMAX)

# Platform defines
if (CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    target_compile_definitions(${PROJECT_NAME} PUBLIC PFN_PLATFORM_WIN32)
    target_compile_definitions(${PROJECT_NAME} PUBLIC VK_USE_PLATFORM_WIN32_KHR)
    target_compile_definitions(${PROJECT_NAME} PUBLIC GLFW_EXPOSE_NATIVE_WIN32)
elseif (CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    target_compile_definitions(${PROJECT_NAME} PUBLIC PFN_PLATFORM_LINUX)
    target_compile_definitions(${PROJECT_NAME} PUBLIC VK_USE_PLATFORM_XLIB_KHR)
    target_compile_definitions(${PROJECT_NAME} PUBLIC GLFW_EXPOSE_NATIVE_X11)
endif()

if (MSVC)
	# Big Object Support (MSCV/Linux)
	target_compile_options(${PROJECT_NAME} PUBLIC /bigobj)
	target_compile_options(${PROJECT_NAME} PUBLIC /arch:AVX2)
	# Multiprocessor (Multithreaded) Compilation Support
	target_compile_options(${PROJECT_NAME} PUBLIC /MP)
	# Enabled updated __cplusplus macro
	target_compile_options(${PROJECT_NAME} PUBLIC "/Zc:__cplusplus")
else ()
	#target_compile_options(${PROJECT_NAME} PUBLIC -Wa, -mbig-obj)
endif ()
