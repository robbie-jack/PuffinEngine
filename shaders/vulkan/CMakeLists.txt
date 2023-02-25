cmake_minimum_required(VERSION 3.24.2)

# set the project name
project(ShaderProject)

find_package(Vulkan REQUIRED)

if (${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "AMD64")
  set(GLSL_VALIDATOR "$ENV{VULKAN_SDK}/Bin/glslangValidator.exe")
else()
  set(GLSL_VALIDATOR "$ENV{VULKAN_SDK}/Bin32/glslangValidator.exe")
endif()

cmake_path(GET CMAKE_CURRENT_SOURCE_DIR PARENT_PATH PUFFIN_SHADER_DIR)
cmake_path(GET PUFFIN_SHADER_DIR PARENT_PATH PUFFIN_DIR)
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PUFFIN_DIR}/bin/vulkan")
file(MAKE_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

set (SPIRV_SOURCE_FILES)
set (SPIRV_BINARY_FILES)

function(build_shader_commands_for_stage shader_dir shader_extension shader_suffix)

	file(GLOB SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/${shader_dir}/*.${shader_extension}")
	
	foreach(SOURCE_FILE ${SOURCE_FILES})
	
		message(STATUS "Building Shader")
		
		get_filename_component(FILE_NAME ${SOURCE_FILE} NAME_WE)
		
		# Make directory if it doesn't exist
		file(MAKE_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${shader_dir}")
		
		set(BINARY_FILE "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${shader_dir}/${FILE_NAME}_${shader_suffix}.spv")
		
		message(STATUS "Source File: ${SOURCE_FILE}")
		message(STATUS "Binary File: ${BINARY_FILE}")
		
		# add_custom_command(
			# OUTPUT ${BINARY_FILE}
			# COMMAND ${GLSL_VALIDATOR} -V ${SOURCE_FILE} -o ${BINARY_FILE}
			# DEPENDS ${SOURCE_FILE})
			
		execute_process(COMMAND ${GLSL_VALIDATOR} -V ${SOURCE_FILE} -o ${BINARY_FILE})
			
		list(APPEND SPIRV_SOURCE_FILES ${SOURCE_FILE})
		list(APPEND SPIRV_BINARY_FILES ${BINARY_FILE})
	
	endforeach(SOURCE_FILE)

endfunction(build_shader_commands_for_stage)
 
function(build_shader_commands shader_dir)

	# Make directory if it doesn't exist
	file(MAKE_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${shader_dir}")

	build_shader_commands_for_stage(${shader_dir} "frag" "fs")
	build_shader_commands_for_stage(${shader_dir} "vert" "vs")

endfunction(build_shader_commands)

build_shader_commands("triangle")
build_shader_commands("forward_shading")

add_custom_target(
    Shaders 
    DEPENDS ${SPIRV_SOURCE_FILES}
	SOURCES ${SPIRV_BINARY_FILES}
    )
	
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT "Shaders")