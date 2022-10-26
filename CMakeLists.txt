cmake_minimum_required(VERSION 3.23)

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
#add_git_submodule("bgfx.cmake")
add_git_submodule("box2d")
add_git_submodule("implot")
add_git_submodule("OpenSimplexNoise")

add_git_submodule("imgui")
checkout_git_branch("imgui" "docking")

add_git_submodule("bgfx.cmake")

# Define CMAKE Variables
set(PROJECT_NAME PuffinEngine)
set(VULKAN_SDK_PATH "C:/VulkanSDK/1.3.216.0")

# Set Project Name
project (${PROJECT_NAME} DESCRIPTION "3D ECS Based Game Engine" LANGUAGES CXX)

# Set C++ Language Standard to C++ 17
set(CMAKE_CXX_STANDARD 17)
set(HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/VulkanTutorial/include)
set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/VulkanTutorial/src)

set(HEADERS 
	# Assets
	${HEADER_DIR}/Assets/Asset.h
	${HEADER_DIR}/Assets/AssetRegistry.h
	${HEADER_DIR}/Assets/MeshAsset.h
	${HEADER_DIR}/Assets/SoundAsset.h
	${HEADER_DIR}/Assets/TextureAsset.h
	${HEADER_DIR}/Assets/Importers/ModelImporter.h
	${HEADER_DIR}/Assets/Importers/TextureImporter.h
	# Audio
	${HEADER_DIR}/Audio/AudioSubsystem.h
	# Components
	${HEADER_DIR}/Components/TransformComponent.h
	${HEADER_DIR}/Components/AngelScriptComponent.h
	${HEADER_DIR}/Components/Rendering/CameraComponent.h
	${HEADER_DIR}/Components/Rendering/LightComponent.h
	${HEADER_DIR}/Components/Rendering/MeshComponent.h
	${HEADER_DIR}/Components/Physics/RigidbodyComponent2D.h
	${HEADER_DIR}/Components/Physics/ShapeComponents2D.h
	${HEADER_DIR}/Components/Physics/VelocityComponent.hpp
	${HEADER_DIR}/Components/Procedural/ProceduralMeshComponent.hpp
	# ECS
	${HEADER_DIR}/ECS/ComponentType.h
	${HEADER_DIR}/ECS/ECS.h
	${HEADER_DIR}/ECS/Entity.h
	${HEADER_DIR}/ECS/EntityID.h
	${HEADER_DIR}/ECS/System.hpp
	# Engine
	${HEADER_DIR}/Engine/Engine.hpp
	${HEADER_DIR}/Engine/Application.hpp
	${HEADER_DIR}/Engine/Subsystem.hpp
	# Input
	${HEADER_DIR}/Input/InputEvent.h
	${HEADER_DIR}/Input/InputSubsystem.h
	# Physics
	${HEADER_DIR}/Physics/CollisionEvent.h
	${HEADER_DIR}/Physics/PuffinPhysics2D/PhysicsTypes2D.h
	${HEADER_DIR}/Physics/PuffinPhysics2D/PhysicsSystem2D.h
	${HEADER_DIR}/Physics/PuffinPhysics2D/PhysicsHelpers2D.h
	${HEADER_DIR}/Physics/PuffinPhysics2D/Shapes/Shape2D.h
	${HEADER_DIR}/Physics/PuffinPhysics2D/Shapes/PolygonShape2D.h
	${HEADER_DIR}/Physics/PuffinPhysics2D/Shapes/CircleShape2D.h
	${HEADER_DIR}/Physics/PuffinPhysics2D/Shapes/BoxShape2D.h
	${HEADER_DIR}/Physics/PuffinPhysics2D/Colliders/Collider2D.h
	${HEADER_DIR}/Physics/PuffinPhysics2D/Colliders/PolygonCollider2D.h
	${HEADER_DIR}/Physics/PuffinPhysics2D/Colliders/BoxCollider2D.h
	${HEADER_DIR}/Physics/PuffinPhysics2D/Colliders/CircleCollider2D.h
	${HEADER_DIR}/Physics/PuffinPhysics2D/Broadphases/Broadphase2D.hpp
	${HEADER_DIR}/Physics/PuffinPhysics2D/Broadphases/PruneAndSweepBroadphase.h
	${HEADER_DIR}/Physics/Box2D/Box2DPhysicsSystem.h
	${HEADER_DIR}/Physics/Box2D/Box2DContactListener.h
	# Procedural
	
	# Rendering
	
	# Scripting
	
	# Types
	
	# UI
	
	# Window
	)

set(SOURCES 
	${SOURCE_DIR}/main.cpp 
	${SOURCE_DIR}/Engine/Engine.cpp)

# Add Main as Executable
add_executable(${PROJECT_NAME} ${HEADERS} ${SOURCES})

message("")
message("============================================================")
message("Sorting Source Files into relative Subfolders")
message("============================================================")

foreach(header IN LISTS HEADERS)
	message("")
	message("Generating Source Group for " ${header})
	file(RELATIVE_PATH relative_path ${HEADER_DIR} ${header})
	get_filename_component(relative_dir ${relative_path} DIRECTORY)
	message("Relative Path: " ${relative_path})
	message("Relative Dir: " ${relative_dir})
    string(REPLACE "/" "\\" relative_path_msvc "${relative_dir}")
    source_group("${relative_path_msvc}" FILES "${header}")
endforeach()

foreach(source IN LISTS SOURCES)
	message("")
	message("Generating Source Group for " ${source})
	file(RELATIVE_PATH relative_path ${SOURCE_DIR} ${source})
	get_filename_component(relative_dir ${relative_path} DIRECTORY)
	message("Relative Path: " ${relative_path})
	message("Relative Dir: " ${relative_dir})
    string(REPLACE "/" "\\" relative_path_msvc "${relative_dir}")
    source_group("${relative_path_msvc}" FILES "${source}")
endforeach()

message("")

# Get & Build Required Libraries
find_package(Vulkan REQUIRED)
CPMAddPackage("gh:nlohmann/json@3.11.2")

target_link_libraries(${PROJECT_NAME} ${Vulkan_LIBRARIES})
target_link_libraries(${PROJECT_NAME} nlohmann_json::nlohmann_json)

target_link_libraries(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/VulkanTutorial/lib/glfw/glfw3.lib)
target_link_libraries(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/VulkanTutorial/lib/irrklang/irrKlang.lib)
target_link_libraries(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/VulkanTutorial/lib/lz4/liblz4.lib)

# Define Debug Libs
target_link_libraries(${PROJECT_NAME} debug ${CMAKE_CURRENT_SOURCE_DIR}/VulkanTutorial/lib/assimp/Debug/assimp-vc142-mtd.lib)
target_link_libraries(${PROJECT_NAME} debug ${CMAKE_CURRENT_SOURCE_DIR}/VulkanTutorial/lib/assimp/Debug/IrrXMLd.lib)

# Define Release Libs
target_link_libraries(${PROJECT_NAME} optimized ${CMAKE_CURRENT_SOURCE_DIR}/VulkanTutorial/lib/assimp/Release/assimp-vc142-mt.lib)
target_link_libraries(${PROJECT_NAME} optimized ${CMAKE_CURRENT_SOURCE_DIR}/VulkanTutorial/lib/assimp/Debug/IrrXML.lib)

# Add include paths
target_include_directories(${PROJECT_NAME} PRIVATE VulkanTutorial/include)
target_include_directories(${PROJECT_NAME} PRIVATE VulkanTutorial/third_party)
target_include_directories(${PROJECT_NAME} PRIVATE ${VULKAN_SDK_PATH}/include)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)

add_compile_definitions(${PROJECT_NAME} PRIVATE PFN_USE_DOUBLE_PRECISION)
option(PFN_USE_DOUBLE_PRECISION "Enabled Double Precision Support" OFF)