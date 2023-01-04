cmake_minimum_required(VERSION 3.24.2)

set(PROJECT_NAME BGFXShaders)

project(${PROJECT_NAME})

set (SHADER_COMPILE_EXE "${CMAKE_CURRENT_SOURCE_DIR}/shaderc.exe")

cmake_path(GET CMAKE_CURRENT_SOURCE_DIR PARENT_PATH SOURCE_PARENT_DIR)
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY "${SOURCE_PARENT_DIR}/bin")
set (CMAKE_SHADER_INCLUDE_COMMON "${SOURCE_PARENT_DIR}/deps/bgfx.cmake/bgfx/examples/common")
set (CMAKE_SHADER_INCLUDE_SRC "${SOURCE_PARENT_DIR}/deps/bgfx.cmake/bgfx/src")

set (SHADER_SOURCE_FILES)
set (SHADER_BINARY_FILES)

function(compile_shaders shader_model shader_model_dir shader_type shader_dir shader_extension)

file(GLOB SOURCE_FILES "${shader_dir}/*.${shader_extension}")

foreach(SOURCE_FILE ${SOURCE_FILES})

	message(STATUS "Building Shader")
	
	get_filename_component(FILE_NAME ${SOURCE_FILE} NAME_WE)
	
	# Make directory if it doesn't exist
	file(MAKE_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${shader_model_dir}/${shader_dir}")
	
	set(BINARY_FILE "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${shader_model_dir}/${shader_dir}/${shader_extension}_${FILE_NAME}.bin")
	set(VARYING_FILE "${CMAKE_CURRENT_SOURCE_DIR}/${shader_dir}/varying.def.sc")
	
	message(STATUS "${SOURCE_FILE}")
	message(STATUS "${BINARY_FILE}")
	message(STATUS "${VARYING_FILE}")
	
	# add_custom_command(
		# OUTPUT ${BINARY_FILE}
		# COMMAND ${SHADER_COMPILE_EXE} 
			# -f ${SOURCE_FILE} 
			# -o ${BINARY_FILE} 
			# -p ${shader_model} 
			# -i ${CMAKE_SHADER_INCLUDE_SRC}
			# -i ${CMAKE_SHADER_INCLUDE_COMMON}
			# --varyingdef ${VARYING_FILE} 
			# --type ${shader_type} 
			# --platform "windows"
		# DEPENDS ${SOURCE_FILE})
		
	execute_process(
		COMMAND ${SHADER_COMPILE_EXE} 
			-f ${SOURCE_FILE} 
			-o ${BINARY_FILE} 
			-p ${shader_model} 
			-i ${CMAKE_SHADER_INCLUDE_SRC}
			-i ${CMAKE_SHADER_INCLUDE_COMMON}
			--varyingdef ${VARYING_FILE} 
			--type ${shader_type} 
			--platform "windows")

	list(APPEND SHADER_SOURCE_FILES ${SOURCE_FILE})
	list(APPEND SHADER_BINARY_FILES ${BINARY_FILE})
	
endforeach(SOURCE_FILE)

endfunction(compile_shaders)

function(compile_all_shaders_in_dir shader_dir shader_model shader_model_dir)

	# Make directory if it doesn't exist
	file(MAKE_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${shader_model_dir}")
	
	compile_shaders("${shader_model}" "${shader_model_dir}" "vertex" "${shader_dir}" "vs")
	compile_shaders("${shader_model}" "${shader_model_dir}" "fragment" "${shader_dir}" "fs")

endfunction(compile_all_shaders_in_dir)

function(compile_all_shaders shader_dir)

	file(GLOB_RECURSE directories LIST_DIRECTORIES true ${shader_dir})
	message(STATUS ${shader_dir})
	
	foreach(directory ${directories})
		message(STATUS ${directory})
	endforeach(directory)

endfunction(compile_all_shaders)

compile_all_shaders_in_dir("forward_shading" "440" "glsl")
compile_all_shaders_in_dir("forward_shading" "spirv" "spirv")
#compile_all_shaders("shaders")

# add_custom_target(
    # Shaders 
    # DEPENDS ${SHADER_BINARY_FILES}
	# SOURCES ${SHADER_SOURCE_FILES}
    # )