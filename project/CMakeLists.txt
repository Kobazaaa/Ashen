#--------------------------------------------------
#    CREATE EXECUTABLE
#--------------------------------------------------
# Source files
set(SOURCES
	# main
	"${SOURCE_DIR}/main.cpp"
	"${SOURCE_DIR}/Renderer.cpp"
	"${SOURCE_DIR}/VulkanContext.cpp"
	"${SOURCE_DIR}/Window.cpp"
)

# Create the executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Set include directories
target_include_directories(${PROJECT_NAME} PRIVATE
	"${SOURCE_DIR}"
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
    glfw
    glm::glm
)
