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
	"${SOURCE_DIR}/rendering/types/Mesh.cpp"
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

include("../cmake/FetchAndIncludeLibraries.cmake")
include("../cmake/CompileShaders.cmake")