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

# Define Add Submodule Function
function(add_git_submodule dir)

find_package(Git REQUIRED)

execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive -- ${dir}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND_ERROR_IS_FATAL ANY)

if(EXISTS ${dir}/CMakeLists.txt)
    add_subdirectory(${dir})
endif()

endfunction(add_git_submodule)

function(checkout_git_branch dir branch)

find_package(Git REQUIRED)

execute_process(COMMAND ${GIT_EXECUTABLE} checkout ${branch}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${dir}
    COMMAND_ERROR_IS_FATAL ANY)

endfunction(checkout_git_branch)

# Init/Update Submodules
add_git_submodule("deps/assimp")
add_git_submodule("deps/bgfx.cmake")
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

checkout_git_branch("deps/imgui" "docking")

# Define CMAKE Variables
set(PROJECT_NAME PuffinEngine)

# Set Project Name
project (${PROJECT_NAME} DESCRIPTION "3D ECS Game Engine" LANGUAGES CXX)

# Set C++ Language Standard to C++ 17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

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
	# Components
	${HEADER_DIR}/Components/TransformComponent.h
	${HEADER_DIR}/Components/SceneObjectComponent.h
	${HEADER_DIR}/Components/Rendering/CameraComponent.h
	${HEADER_DIR}/Components/Rendering/LightComponent.h
	${HEADER_DIR}/Components/Rendering/MeshComponent.h
	${HEADER_DIR}/Components/Physics/RigidbodyComponent2D.h
	${HEADER_DIR}/Components/Physics/ShapeComponents2D.h
	${HEADER_DIR}/Components/Physics/VelocityComponent.h
	${HEADER_DIR}/Components/Procedural/ProceduralMeshComponent.h
	${HEADER_DIR}/Components/Scripting/AngelScriptComponent.h
	# ECS
	${HEADER_DIR}/ECS/EnTTSubsystem.h
	# Core
	${HEADER_DIR}/Core/Engine.h
	${HEADER_DIR}/Core/Application.h
	${HEADER_DIR}/Core/Subsystem.h
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
	# Rendering BGFX
	#${HEADER_DIR}/Rendering/BGFX/BGFXRenderSystem.h
	#${HEADER_DIR}/Rendering/BGFX/BGFXVertex.h
	#${HEADER_DIR}/Rendering/BGFX/BGFXTypes.h
	#${HEADER_DIR}/Rendering/BGFX/BGFXTextureArray.h
	# Rendering Vulkan
	${HEADER_DIR}/Rendering/Vulkan/VKRenderSystem.h
	${HEADER_DIR}/Rendering/Vulkan/VKTypes.h
	${HEADER_DIR}/Rendering/Vulkan/VKVertex.h
	${HEADER_DIR}/Rendering/Vulkan/VKHelpers.h
	${HEADER_DIR}/Rendering/Vulkan/VKDescriptors.h
	${HEADER_DIR}/Rendering/Vulkan/VKPipeline.h
	${HEADER_DIR}/Rendering/Vulkan/VKUnifiedGeometryBuffer.h
	# Scripting
	${HEADER_DIR}/Scripting/RegisterTypeHelpers.h
	${HEADER_DIR}/Scripting/AngelScriptSystem.h
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
	# ECS
	
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
	# Rendering BGFX
	#${SOURCE_DIR}/Rendering/BGFX/BGFXRenderSystem.cpp
	# Rendering Vulkan
	${SOURCE_DIR}/Rendering/Vulkan/VKRenderSystem.cpp
	${SOURCE_DIR}/Rendering/Vulkan/VKHelpers.cpp
	${SOURCE_DIR}/Rendering/Vulkan/VKDescriptors.cpp
	${SOURCE_DIR}/Rendering/Vulkan/VKUnifiedGeometryBuffer.cpp
	# Scripting
	${SOURCE_DIR}/Scripting/AngelScriptSystem.cpp
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
	${SOURCE_DIR}/main.cpp 
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

# Add Main as Executable
add_executable(${PROJECT_NAME} ${HEADERS} ${SOURCES})
target_sources(${PROJECT_NAME} PRIVATE ${IMGUI_SOURCES})
target_sources(${PROJECT_NAME} PRIVATE ${OPENSIMPLEX_SOURCES})
target_sources(${PROJECT_NAME} PRIVATE ${VKBOOTSTRAP_SOURCES})
target_sources(${PROJECT_NAME} PRIVATE ${ANGELSCRIPT_SOURCES})
target_sources(${PROJECT_NAME} PRIVATE ${ANGELSCRIPT_SOURCES})
target_sources(${PROJECT_NAME} PRIVATE ${ENKITS_SOURCES})

message("")
message("============================================================")
message("Sorting Source Files into relative Subfolders")
message("============================================================")

function(sort_into_source_group files dir sort_dir)

foreach(file IN LISTS ${files})
	#message("")
	message("Generating Source Group for " ${file})
	file(RELATIVE_PATH relative_path ${dir} ${file})
	get_filename_component(relative_dir ${relative_path} DIRECTORY)
	#message("Relative Path: " ${relative_path})
	#message("Relative Dir: " ${relative_dir})
    string(REPLACE "/" "\\" relative_path_msvc "${relative_dir}")
    source_group("${sort_dir}/${relative_path_msvc}" FILES "${file}")
endforeach()

endfunction(sort_into_source_group)

sort_into_source_group(HEADERS ${HEADER_DIR} "")
sort_into_source_group(SOURCES ${SOURCE_DIR} "")
sort_into_source_group(IMGUI_SOURCES ${IMGUI_DIR} Libraries/ImGui)
sort_into_source_group(OPENSIMPLEX_SOURCES ${OPENSIMPLEX_DIR} Libraries/OpenSimplexNoise)
sort_into_source_group(VKBOOTSTRAP_SOURCES ${VKBOOTSTRAP_DIR} Libraries/VKBootstrap)
sort_into_source_group(ANGELSCRIPT_SOURCES ${ANGELSCRIPT_DIR} Libraries/AngelScript)
sort_into_source_group(ENKITS_SOURCES ${ENKITS_DIR} Libraries/EnkiTS)

message("")

# Get & Build Required Libraries
find_package(Vulkan REQUIRED)
CPMAddPackage("gh:nlohmann/json@3.11.2")

# Include Jolt
FetchContent_Declare(
        JoltPhysics
        GIT_REPOSITORY "https://github.com/jrouwe/JoltPhysics"
        GIT_TAG "0436c32dfd83d26f7559e260378d7fa1d7962841"
		SOURCE_SUBDIR "Build"
)
FetchContent_MakeAvailable(JoltPhysics)

target_link_libraries(${PROJECT_NAME} ${Vulkan_LIBRARIES})
target_link_libraries(${PROJECT_NAME} nlohmann_json::nlohmann_json)
target_link_libraries(${PROJECT_NAME} Jolt)

set(LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib)

target_link_libraries(${PROJECT_NAME} ${LIB_DIR}/irrKlang/irrKlang.lib)

# Define Debug Libs
target_link_libraries(${PROJECT_NAME} debug ${LIB_DIR}/box2d/Debug/box2d.lib)
#target_link_libraries(${PROJECT_NAME} debug ${LIB_DIR}/assimp/Debug/assimp-vc143-mtd.lib)
target_link_libraries(${PROJECT_NAME} debug ${LIB_DIR}/glfw/Debug/glfw3.lib)
target_link_libraries(${PROJECT_NAME} debug ${LIB_DIR}/angelscript/Debug/angelscript64d.lib)
target_link_libraries(${PROJECT_NAME} debug ${LIB_DIR}/lz4/Debug/liblz4_static.lib)

#target_link_libraries(${PROJECT_NAME} debug ${LIB_DIR}/bgfx/Debug/bgfx.lib)
#target_link_libraries(${PROJECT_NAME} debug ${LIB_DIR}/bgfx/Debug/bimg.lib)
#target_link_libraries(${PROJECT_NAME} debug ${LIB_DIR}/bgfx/Debug/bx.lib)
#target_link_libraries(${PROJECT_NAME} debug ${LIB_DIR}/bgfx/Debug/astc-encoder.lib)
#target_link_libraries(${PROJECT_NAME} debug ${LIB_DIR}/bgfx/Debug/edtaa3.lib)
#target_link_libraries(${PROJECT_NAME} debug ${LIB_DIR}/bgfx/Debug/etc1.lib)
#target_link_libraries(${PROJECT_NAME} debug ${LIB_DIR}/bgfx/Debug/etc2.lib)
#target_link_libraries(${PROJECT_NAME} debug ${LIB_DIR}/bgfx/Debug/iqa.lib)
#target_link_libraries(${PROJECT_NAME} debug ${LIB_DIR}/bgfx/Debug/nvtt.lib)
#target_link_libraries(${PROJECT_NAME} debug ${LIB_DIR}/bgfx/Debug/pvrtc.lib)
#target_link_libraries(${PROJECT_NAME} debug ${LIB_DIR}/bgfx/Debug/squish.lib)
#target_link_libraries(${PROJECT_NAME} debug ${LIB_DIR}/bgfx/Debug/tinyexr.lib)

# Define Release Libs
target_link_libraries(${PROJECT_NAME} optimized ${LIB_DIR}/box2d/Release/box2d.lib)
#target_link_libraries(${PROJECT_NAME} optimized ${LIB_DIR}/assimp/Release/assimp-vc143-mt.lib)
target_link_libraries(${PROJECT_NAME} optimized ${LIB_DIR}/glfw/Release/glfw3.lib)
target_link_libraries(${PROJECT_NAME} optimized ${LIB_DIR}/angelscript/Release/angelscript64.lib)
target_link_libraries(${PROJECT_NAME} optimized ${LIB_DIR}/lz4/Release/liblz4_static.lib)

#target_link_libraries(${PROJECT_NAME} optimized ${LIB_DIR}/bgfx/Release/bgfx.lib)
#target_link_libraries(${PROJECT_NAME} optimized ${LIB_DIR}/bgfx/Release/bimg.lib)
#target_link_libraries(${PROJECT_NAME} optimized ${LIB_DIR}/bgfx/Release/bx.lib)
#target_link_libraries(${PROJECT_NAME} optimized ${LIB_DIR}/bgfx/Release/astc-encoder.lib)
#target_link_libraries(${PROJECT_NAME} optimized ${LIB_DIR}/bgfx/Release/edtaa3.lib)
#target_link_libraries(${PROJECT_NAME} optimized ${LIB_DIR}/bgfx/Release/etc1.lib)
#target_link_libraries(${PROJECT_NAME} optimized ${LIB_DIR}/bgfx/Release/etc2.lib)
#target_link_libraries(${PROJECT_NAME} optimized ${LIB_DIR}/bgfx/Release/iqa.lib)
#target_link_libraries(${PROJECT_NAME} optimized ${LIB_DIR}/bgfx/Release/nvtt.lib)
#target_link_libraries(${PROJECT_NAME} optimized ${LIB_DIR}/bgfx/Release/pvrtc.lib)
#target_link_libraries(${PROJECT_NAME} optimized ${LIB_DIR}/bgfx/Release/squish.lib)
#target_link_libraries(${PROJECT_NAME} optimized ${LIB_DIR}/bgfx/Release/tinyexr.lib)

set(DEPS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps)

# Add include paths
target_include_directories(${PROJECT_NAME} PRIVATE include)
target_include_directories(${PROJECT_NAME} PRIVATE third_party)
target_include_directories(${PROJECT_NAME} PRIVATE third_party/angelscript)
target_include_directories(${PROJECT_NAME} PRIVATE $ENV{VULKAN_SDK}/include)
target_include_directories(${PROJECT_NAME} PRIVATE ${DEPS_DIR}/box2d/include)
target_include_directories(${PROJECT_NAME} PRIVATE ${DEPS_DIR}/OpenSimplexNoise)
target_include_directories(${PROJECT_NAME} PRIVATE ${DEPS_DIR}/imgui)
target_include_directories(${PROJECT_NAME} PRIVATE ${DEPS_DIR}/imgui-filebrowser)
target_include_directories(${PROJECT_NAME} PRIVATE ${DEPS_DIR}/lz4/lib)
target_include_directories(${PROJECT_NAME} PRIVATE ${DEPS_DIR}/vma-hpp/VulkanMemoryAllocator/include)
target_include_directories(${PROJECT_NAME} PRIVATE ${DEPS_DIR}/vma-hpp/include)
target_include_directories(${PROJECT_NAME} PRIVATE ${DEPS_DIR}/vkb/src)
target_include_directories(${PROJECT_NAME} PRIVATE ${DEPS_DIR}/tinygltf)
#target_include_directories(${PROJECT_NAME} PRIVATE ${DEPS_DIR}/bgfx.cmake/bgfx/include)
#target_include_directories(${PROJECT_NAME} PRIVATE ${DEPS_DIR}/bgfx.cmake/bx/compat/msvc)
#target_include_directories(${PROJECT_NAME} PRIVATE ${DEPS_DIR}/bgfx.cmake/bx/compat/mingw)
#target_include_directories(${PROJECT_NAME} PRIVATE ${DEPS_DIR}/bgfx.cmake/bx/include)
#target_include_directories(${PROJECT_NAME} PRIVATE ${DEPS_DIR}/bgfx.cmake/bimg/include)
target_include_directories(${PROJECT_NAME} PRIVATE ${DEPS_DIR}/enkits/src)
target_include_directories(${PROJECT_NAME} PRIVATE ${DEPS_DIR}/entt/src)
#target_include_directories(${PROJECT_NAME} PRIVATE ${DEPS_DIR}/assimp/include)
target_include_directories(${PROJECT_NAME} PRIVATE ${JoltPhysics_SOURCE_DIR}/..)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)

target_compile_definitions(${PROJECT_NAME} PRIVATE PFN_USE_DOUBLE_PRECISION)
option(PFN_USE_DOUBLE_PRECISION "Enabled Double Precision Support" ON)

set_property(TARGET ${PROJECT_NAME} PROPERTY 
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
	
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

# Defines to get bx compilation working
#target_compile_definitions(${PROJECT_NAME} PRIVATE BX_CONFIG_DEBUG)

target_compile_definitions(${PROJECT_NAME} PRIVATE GLM_FORCE_DEFAULT_ALIGNED_GENTYPES)
target_compile_definitions(${PROJECT_NAME} PRIVATE GLM_FORCE_SIMD_AVX2)

# Platform defines (WIN32)
if (WIN32)
	target_compile_definitions(${PROJECT_NAME} PRIVATE PFN_PLATFORM_WIN32)
	target_compile_definitions(${PROJECT_NAME} PRIVATE VK_USE_PLATFORM_WIN32_KHR)
	target_compile_definitions(${PROJECT_NAME} PRIVATE NOMINMAX)
	target_compile_definitions(${PROJECT_NAME} PRIVATE GLFW_EXPOSE_NATIVE_WIN32)
else()
	target_compile_definitions(${PROJECT_NAME} PRIVATE PFN_PLATFORM_LINUX)
	target_compile_definitions(${PROJECT_NAME} PRIVATE GLFW_EXPOSE_NATIVE_X11)
endif()

if (MSVC)
	# Big Object Support (MSCV/Linux)
	target_compile_options(${PROJECT_NAME} PRIVATE /bigobj)
	target_compile_options(${PROJECT_NAME} PRIVATE /arch:AVX2)
	# Multiprocessor (Multithreaded) Compilation Support
	target_compile_options(${PROJECT_NAME} PRIVATE /MP)
	# Enabled updated __cplusplus macro (needed for bx compilation)
	target_compile_options(${PROJECT_NAME} PRIVATE "/Zc:__cplusplus")
else ()
	#target_compile_options(${PROJECT_NAME} PRIVATE -Wa, -mbig-obj)
endif ()

# Jolt Config

# When turning this option on, the library will be compiled using doubles for positions. This allows for much bigger worlds.
set(DOUBLE_PRECISION PFN_USE_DOUBLE_PRECISION)

# When turning this option on, the library will be compiled with debug symbols
set(GENERATE_DEBUG_SYMBOLS ON)

# When turning this option on, the library will be compiled in such a way to attempt to keep the simulation deterministic across platforms
set(CROSS_PLATFORM_DETERMINISTIC OFF)

# When turning this option on, the library will be compiled with interprocedural optimizations enabled, also known as link-time optimizations or link-time code generation.
# Note that if you turn this on you need to use SET_INTERPROCEDURAL_OPTIMIZATION() or set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON) to enable LTO specificly for your own project as well.
# If you don't do this you may get an error: /usr/bin/ld: libJolt.a: error adding symbols: file format not recognized
set(INTERPROCEDURAL_OPTIMIZATION ON)

# When turning this on, in Debug and Release mode, the library will emit extra code to ensure that the 4th component of a 3-vector is kept the same as the 3rd component 
# and will enable floating point exceptions during simulation to detect divisions by zero. 
# Note that this currently only works using MSVC. Clang turns Float2 into a SIMD vector sometimes causing floating point exceptions (the option is ignored).
set(FLOATING_POINT_EXCEPTIONS_ENABLED OFF)

# Number of bits to use in ObjectLayer. Can be 16 or 32.
set(OBJECT_LAYER_BITS 16)

# Select X86 processor features to use, by default the library compiles with AVX2, if everything is off it will be SSE2 compatible.
set(USE_SSE4_1 ON)
set(USE_SSE4_2 ON)
set(USE_AVX ON)
set(USE_AVX2 ON)
set(USE_AVX512 OFF)
set(USE_LZCNT ON)
set(USE_TZCNT ON)
set(USE_F16C ON)
set(USE_FMADD ON)
