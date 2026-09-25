cmake_minimum_required(VERSION 3.11)

project(RenderingTechniques)


set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory(${PROJECT_SOURCE_DIR}/libs/glfw)
add_subdirectory(${PROJECT_SOURCE_DIR}/libs/pugixml)
add_subdirectory(${PROJECT_SOURCE_DIR}/libs/tinyobjloader)

find_package(Vulkan REQUIRED)


include_directories(
	${Vulkan_INCLUDE_DIRS}
	include
	libs
	libs/tinyformat
	libs/filesystem
	libs/glm
	libs/pugixml/src
	libs/tinyobjloader
	)


add_executable(RenderingTechniques 
#INCLUDES
include/defines.h
include/common.h
include/scene.h
include/engine.h
include/camera.h
include/light.h
include/entity.h
include/meshRegistry.h
include/material.h
include/diffuse.h
include/microfacets.h
include/transform.h
include/runtime.h
include/frame.h
include/shaderRegistry.h
include/input.h
include/renderer.h

# VULKAN
include/vulkan/utilsVK.h
include/vulkan/rendererVK.h
include/vulkan/deviceVK.h
include/vulkan/windowVK.h
include/vulkan/meshVK.h
include/vulkan/extensionsVK.h

#render passes
include/vulkan/renderPassVK.h
include/vulkan/deferredPassVK.h
include/vulkan/compositionPassVK.h
include/vulkan/blurPassVK.h
include/vulkan/denoisePassVK.h
include/vulkan/depthPassVK.h
include/vulkan/lightingPassVK.h
include/vulkan/motionVectorsPassVK.h
include/vulkan/ppPassVK.h
include/vulkan/shadowPassVK.h
include/vulkan/shadowPassVK_RTX.h
include/vulkan/ssaoPassVK.h
include/vulkan/taaPassVK.h



#CPPS
main.cpp
src/entity.cpp
src/engine.cpp
src/common.cpp
src/camera.cpp
src/light.cpp
src/scene.cpp
src/material.cpp
src/diffuse.cpp
src/microfacets.cpp
src/transform.cpp
src/meshRegistry.cpp
src/shaderRegistry.cpp
src/runtime.cpp

# VULKAN
src/vulkan/utilsVK.cpp
src/vulkan/rendererVK.cpp
src/vulkan/windowVK.cpp
src/vulkan/deviceVK.cpp
src/vulkan/meshVK.cpp
src/vulkan/extensionsVK.cpp

#render passes
src/vulkan/deferredPassVK.cpp
src/vulkan/compositionPassVK.cpp
src/vulkan/blurPassVK.cpp
src/vulkan/denoisePassVK.cpp
src/vulkan/dephtPassVK.cpp
src/vulkan/lightingPassVK.cpp
src/vulkan/motionVectorsPassVK.cpp
src/vulkan/ppPassVK.cpp
src/vulkan/shadowPassVK.cpp
src/vulkan/shadowPassVK_RTX.cpp
src/vulkan/ssaoPassVK.cpp
src/vulkan/taaPassVK.cpp
)


set_target_properties(RenderingTechniques PROPERTIES 
VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
VS_DEBUGGER_COMMAND_ARGUMENTS "$(SolutionDir)../scenes/shadows/shadows.xml"
)
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT RenderingTechniques)
target_link_libraries(RenderingTechniques glfw pugixml::pugixml ${Vulkan_LIBRARIES} tinyobjloader )
