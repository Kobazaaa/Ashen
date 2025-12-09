#--------------------------------------------------
#    CREATE EXECUTABLE
#--------------------------------------------------
# Source files
set(SOURCES
	# main
	"${SOURCE_DIR}/main.cpp"
	# helpers
	"${SOURCE_DIR}/helpers/Timer.cpp"
	# misc
	"${SOURCE_DIR}/misc/Camera.cpp"
	"${SOURCE_DIR}/misc/Window.cpp"
	# rendering
	"${SOURCE_DIR}/rendering/pipeline/Pipeline.cpp"
	"${SOURCE_DIR}/rendering/Renderer.cpp"
	"${SOURCE_DIR}/rendering/VulkanContext.cpp"
)

# Create the executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Set include directories
target_include_directories(${PROJECT_NAME} PRIVATE
	"${SOURCE_DIR}"
	"${SOURCE_DIR}/helpers"
	"${SOURCE_DIR}/misc"
	"${SOURCE_DIR}/rendering"
	"${SOURCE_DIR}/rendering/pipeline"
	"${SOURCE_DIR}/rendering/types"
)

# Max Warning Level & Warnings as Errors
target_compile_options(${PROJECT_NAME} PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Werror>
)

#--------------------------------------------------
#    LIBRARIES
#--------------------------------------------------
target_link_libraries(${PROJECT_NAME} PRIVATE vk-bootstrap)

find_package(Vulkan REQUIRED)
if(Vulkan_FOUND)
    MESSAGE("Vulkan Found!")
endif()

include(FetchContent)

# Fetch GLFW
FetchContent_Declare(
   GLFW
   GIT_REPOSITORY https://github.com/glfw/glfw.git
   GIT_TAG 3.4
   GIT_SHALLOW TRUE
   GIT_PROGRESS TRUE
)

# Fetch GLM
FetchContent_Declare(
   GLM
   GIT_REPOSITORY https://github.com/g-truc/glm.git
   GIT_TAG 1.0.1
   GIT_SHALLOW TRUE
   GIT_PROGRESS TRUE
)

FetchContent_MakeAvailable(GLFW GLM)

# Link libraries to the project
target_link_libraries(${PROJECT_NAME} PUBLIC
	Vulkan::Vulkan
	glfw
    glm::glm
)


#--------------------------------------------------
#    SHADER COMPILATION
#--------------------------------------------------

# Check if Vulkan SDK has already been found, if not, shaders cannot be compiled
if(NOT Vulkan_FOUND)
    message(FATAL_ERROR "Vulkan not found yet, cannot compile shaders!")
endif()

# Find the glslc executable
find_program(GLSLC_EXECUTABLE glslc HINTS "${Vulkan_GLSLC_EXECUTABLE}")
if(NOT GLSLC_EXECUTABLE)
    message(FATAL_ERROR "glslc not found! Make sure Vulkan SDK is installed and available in PATH.")
else()
    message("glslc.exe found at ${GLSLC_EXECUTABLE}")
endif()

# Set shader directories
set(SHADER_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
set(SHADER_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/shaders)
file(MAKE_DIRECTORY ${SHADER_BINARY_DIR})

# Shader Files
file(GLOB SHADER_SOURCES
  ${SHADER_SOURCE_DIR}/*.vert
  ${SHADER_SOURCE_DIR}/*.frag
  ${SHADER_SOURCE_DIR}/*.comp
)

# Compile the shaders
add_custom_target(CompileShaders ALL COMMENT "Compiling shaders to output directory.")
foreach(SHADER ${SHADER_SOURCES})
    get_filename_component(SHADER_NAME ${SHADER} NAME)
    set(SHADER_OUTPUT_NAME "${SHADER_NAME}.spv")
    set(SHADER_OUTPUT_DIR "${SHADER_BINARY_DIR}/${SHADER_OUTPUT_NAME}")

    message("Compiling ${SHADER_NAME} to ${SHADER_OUTPUT_NAME}")

    add_custom_command(
        TARGET CompileShaders POST_BUILD
        COMMAND ${GLSLC_EXECUTABLE} -g ${SHADER} -o ${SHADER_OUTPUT_DIR} -I${SHADER_SOURCE_DIR}
        DEPENDS ${SHADER}
        COMMENT "Compiling ${SHADER_INPUT_NAME} to ${SHADER_OUTPUT_NAME}.spv"
        VERBATIM
    )
endforeach()

# Add CompileShaders as a dependency to project
add_dependencies(${PROJECT_NAME} CompileShaders)
