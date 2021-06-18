cmake_minimum_required (VERSION 3.18)
set (CMAKE_CXX_STANDARD 20)
project ("WorkingTitle4")

file (GLOB SRC_SHADER_COMMON CONFIGURE_DEPENDS "src/shader/*common.glsl") # hacky glob, instead of include analysis
file (GLOB SRC_SHADER_PROG CONFIGURE_DEPENDS "src/shader/*.*.glsl")

foreach (GLSL_FILE IN LISTS SRC_SHADER_PROG)
  string (REPLACE ${CMAKE_SOURCE_DIR}/src/shader/ "" NAME_FILE ${GLSL_FILE})
  string (REPLACE ".glsl" "" NAME_DOT ${NAME_FILE})
  string (REPLACE "." "_" NAME_UNDERSCORE ${NAME_DOT})
  string (CONCAT SPV_FILE ${CMAKE_BINARY_DIR}/generated/spv/${NAME_DOT}.spv)
  string (CONCAT EMBED_FILE ${CMAKE_BINARY_DIR}/generated/embed/${NAME_UNDERSCORE}.c)

  add_custom_command (
    OUTPUT ${SPV_FILE}
    COMMAND glslangValidator ${GLSL_FILE} --target-env vulkan1.2 -o ${SPV_FILE}
    # COMMAND glslc ${GLSL_FILE} -o ${SPV_FILE}
    DEPENDS ${GLSL_FILE} ${SRC_SHADER_COMMON}
  )

  add_custom_command (
    OUTPUT ${EMBED_FILE}
    COMMAND embedfile ${NAME_UNDERSCORE} ${SPV_FILE}
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/generated/embed"
    DEPENDS embedfile ${SPV_FILE}
  )
endforeach()

set(ALL_EMBEDDED_GLSL ${SRC_SHADER_PROG})
list (TRANSFORM ALL_EMBEDDED_GLSL REPLACE "\\." "_")
list (TRANSFORM ALL_EMBEDDED_GLSL REPLACE "_glsl" ".c")
list (TRANSFORM ALL_EMBEDDED_GLSL REPLACE ${CMAKE_SOURCE_DIR}/src/shader ${CMAKE_BINARY_DIR}/generated/embed)

file (GLOB_RECURSE SRC_LIB CONFIGURE_DEPENDS "src/lib/*.cxx")
file (GLOB_RECURSE SRC_ENGINE CONFIGURE_DEPENDS "src/engine/*.cxx")

add_subdirectory(submodule/fmt)

include_directories (
  $ENV{CUSTOM_INCLUDE_PATH}
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/submodule/imgui
  ${CMAKE_SOURCE_DIR}/submodule/glm
  ${CMAKE_SOURCE_DIR}/submodule/tracy
  ${CMAKE_SOURCE_DIR}/submodule/tinygltf
  ${CMAKE_SOURCE_DIR}/submodule/fmt/include
)

add_executable (
  embedfile
  "src/exe/embedfile.cxx"
)

add_executable (
  cubewriter
  "src/exe/cubewriter.cxx"
)

add_executable (
  voxelconverter
  "src/exe/voxelconverter.cxx"
)

add_executable (
  taskmark
  "src/exe/taskmark.cxx"
  "src/impl/global_overload.cxx"
  "src/lib/task.cxx"
  "${CMAKE_SOURCE_DIR}/submodule/tracy/TracyClient.cpp"
)
target_compile_definitions(taskmark PUBLIC TRACY_ENABLE=1)
target_compile_definitions(taskmark PUBLIC TRACY_NO_EXIT=1)
target_link_libraries(taskmark fmt::fmt-header-only)

add_executable (
  engine
  WIN32
  "src/impl/global_overload.cxx"
  "src/impl/tiny_gltf.cxx"
  "src/impl/stb_image.cxx"
  "src/impl/stb_image_write.cxx"
  ${SRC_LIB}
  ${SRC_ENGINE}
  ${ALL_EMBEDDED_GLSL}
  "${CMAKE_SOURCE_DIR}/submodule/tracy/TracyClient.cpp"
  "${CMAKE_SOURCE_DIR}/submodule/imgui/imgui.cpp"
  "${CMAKE_SOURCE_DIR}/submodule/imgui/imgui_draw.cpp"
  "${CMAKE_SOURCE_DIR}/submodule/imgui/imgui_tables.cpp"
  "${CMAKE_SOURCE_DIR}/submodule/imgui/imgui_widgets.cpp"
  "${CMAKE_SOURCE_DIR}/submodule/imgui/imgui_demo.cpp"
  "${CMAKE_SOURCE_DIR}/submodule/imgui/backends/imgui_impl_glfw.cpp"
  "${CMAKE_SOURCE_DIR}/submodule/imgui/backends/imgui_impl_vulkan.cpp"
)
target_compile_definitions(engine PUBLIC -DWINDOWS)
# target_compile_definitions(engine PUBLIC -DWIN32_LEAN_AND_MEAN)
target_compile_definitions(engine PUBLIC -DVC_EXTRALEAN)
target_compile_definitions(engine PUBLIC -DGLFW_INCLUDE_VULKAN)
target_compile_definitions(engine PUBLIC TRACY_ENABLE=1)
target_compile_definitions(engine PUBLIC TRACY_ON_DEMAND=1)
# target_compile_definitions(engine PUBLIC TRACY_NO_EXIT=1)
target_link_directories (engine PUBLIC $ENV{CUSTOM_LINK_PATH})
target_link_libraries (engine glfw3 vulkan-1) 
target_link_libraries(engine fmt::fmt-header-only)