cmake_minimum_required (VERSION 3.18)
set (CMAKE_CXX_STANDARD 20)
project ("WorkingTitle4")

message(CUSTOM_LINK_PATH = $ENV{CUSTOM_LINK_PATH})
message(CUSTOM_INCLUDE_PATH = $ENV{CUSTOM_INCLUDE_PATH})

file (GLOB SRC_SHADER_COMMON CONFIGURE_DEPENDS "src/shader/common/*.glsl") # hacky glob, instead of include analysis
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
file (GLOB_RECURSE SRC_TASK CONFIGURE_DEPENDS "src/task/*.cxx")
file (GLOB_RECURSE SRC_TOOLS CONFIGURE_DEPENDS "src/tools/*.cxx")

add_subdirectory(submodule/fmt)

include_directories(
  SYSTEM
  $ENV{CUSTOM_INCLUDE_PATH}
  ${CMAKE_SOURCE_DIR}/submodule/imgui
  ${CMAKE_SOURCE_DIR}/submodule/glm
  ${CMAKE_SOURCE_DIR}/submodule/tracy
  ${CMAKE_SOURCE_DIR}/submodule/tinygltf
  ${CMAKE_SOURCE_DIR}/submodule/fmt/include
  ${CMAKE_SOURCE_DIR}/submodule/nativefiledialog/src/include
)

include_directories (
  ${CMAKE_SOURCE_DIR}
)

add_library (
  engine_lib
  STATIC
  "src/impl/global_overload.cxx"
  "src/impl/log.cxx"
  "src/impl/stb_image.cxx"
  "src/impl/stb_image_write.cxx"
  "src/impl/tinygltf.cxx"
  ${SRC_LIB}
  ${SRC_ENGINE}
  ${SRC_TASK}
  ${SRC_TOOLS}
  ${ALL_EMBEDDED_GLSL}
  "${CMAKE_SOURCE_DIR}/submodule/tracy/TracyClient.cpp"
  "${CMAKE_SOURCE_DIR}/submodule/imgui/imgui.cpp"
  "${CMAKE_SOURCE_DIR}/submodule/imgui/imgui_draw.cpp"
  "${CMAKE_SOURCE_DIR}/submodule/imgui/imgui_tables.cpp"
  "${CMAKE_SOURCE_DIR}/submodule/imgui/imgui_widgets.cpp"
  "${CMAKE_SOURCE_DIR}/submodule/imgui/imgui_demo.cpp"
  "${CMAKE_SOURCE_DIR}/submodule/imgui/backends/imgui_impl_glfw.cpp"
  "${CMAKE_SOURCE_DIR}/submodule/imgui/backends/imgui_impl_vulkan.cpp"
  "${CMAKE_SOURCE_DIR}/submodule/imgui/misc/cpp/imgui_stdlib.cpp"
)

target_compile_definitions(engine_lib PUBLIC -DENGINE_DEVELOPER)
target_compile_definitions(engine_lib PUBLIC -DWINDOWS)
# target_compile_definitions(engine_lib PUBLIC -DWIN32_LEAN_AND_MEAN)
target_compile_definitions(engine_lib PUBLIC -DVC_EXTRALEAN)
target_compile_definitions(engine_lib PUBLIC -DGLFW_INCLUDE_VULKAN)
target_compile_definitions(engine_lib PUBLIC -DGLM_FORCE_DEPTH_ZERO_TO_ONE)
target_compile_definitions(engine_lib PUBLIC TRACY_ENABLE=1)
target_compile_definitions(engine_lib PUBLIC TRACY_VULKAN_ENABLE=1)
target_compile_definitions(engine_lib PUBLIC IMGUI_DISABLE_OBSOLETE_FUNCTIONS=1)
# target_compile_definitions(engine_lib PUBLIC TRACY_ON_DEMAND=1)
# target_compile_definitions(engine_lib PUBLIC TRACY_NO_EXIT=1)
target_link_directories (engine_lib PUBLIC $ENV{CUSTOM_LINK_PATH})
target_link_directories (engine_lib PUBLIC "${CMAKE_SOURCE_DIR}/submodule/nativefiledialog/build/lib/Release/x64")
target_link_libraries (engine_lib glfw3)
target_link_libraries (engine_lib vulkan-1)
target_link_libraries (engine_lib nfd)
target_link_libraries(engine_lib fmt::fmt-header-only)



# executables

file (GLOB SRC_EXE_SCRIPTS CONFIGURE_DEPENDS "src/exe/scripts/*.cxx")
foreach (FILE IN LISTS SRC_EXE_SCRIPTS)
  string (REPLACE ${CMAKE_SOURCE_DIR}/src/exe/scripts/ "" EXE_NAME ${FILE})
  string (REPLACE ".cxx" "" EXE_NAME ${EXE_NAME})

  add_executable (
    script_${EXE_NAME}
    WIN32
    ${FILE}
  )

  target_link_libraries(script_${EXE_NAME} engine_lib)
endforeach()

add_executable (
  engine
  WIN32
  "src/exe/engine.cxx"
)
target_link_libraries(engine engine_lib)

add_executable (
  embedfile
  "src/exe/embedfile.cxx"
)

add_executable (
  taskmark
  "src/exe/taskmark.cxx"
  "src/impl/global_overload.cxx"
  "src/impl/log.cxx"
  "src/lib/task.cxx"
  "${CMAKE_SOURCE_DIR}/submodule/tracy/TracyClient.cpp"
)
target_compile_definitions(taskmark PUBLIC TRACY_ENABLE=1)
target_compile_definitions(taskmark PUBLIC TRACY_NO_EXIT=1)
target_link_libraries(taskmark fmt::fmt-header-only)

set_target_properties(
    script_sponza PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
