cmake_minimum_required(VERSION 3.8 FATAL_ERROR)

project(Sequentity CXX)

get_filename_component(PROJECT_DIR "${CMAKE_CURRENT_SOURCE_DIR}" ABSOLUTE)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CONFIGURATION "Debug")
else()
    # Support alternative release configurations, e.g. RelWithDebInfo
    set(CONFIGURATION "Release")
endif()

include_directories(
    ${CMAKE_SOURCE_DIR}/External/magnum/x64-windows-vsclang-static/${CONFIGURATION}/include
    ${CMAKE_SOURCE_DIR}/External/magnum/x64-windows-vsclang-static/${CONFIGURATION}/include/MagnumExternal/OpenGL
    ${CMAKE_SOURCE_DIR}/External/magnum-singles
    ${CMAKE_SOURCE_DIR}/External/imgui/x64-windows/include
    ${CMAKE_SOURCE_DIR}/External/entt
    ${CMAKE_SOURCE_DIR}/../
)


link_directories(
    ${CMAKE_SOURCE_DIR}/External/magnum/x64-windows-vs2015-static/${CONFIGURATION}/lib
)


set(SRC_FILES
    Source/_sqty.cpp
    Source/main.cpp
)

set(HEADERS_FILES
    ../Sequentity.h
)

source_group("Sources" FILES ${SRC_FILES})
source_group("Headers" FILES ${HEADERS_FILES})

message("${PROJECT_NAME}: Main project name: ${CMAKE_PROJECT_NAME}")
message("${PROJECT_NAME}: Current source dir: ${CMAKE_CURRENT_SOURCE_DIR}")
message("${PROJECT_NAME}: Current binary dir: ${CMAKE_CURRENT_BINARY_DIR}")
message("${PROJECT_NAME}: Source dir: ${CMAKE_SOURCE_DIR}")
message("${PROJECT_NAME}: Build type: ${CMAKE_BUILD_TYPE}")
message("${PROJECT_NAME}: Configuration: ${CONFIGURATION}")

# Magnum's got a lot of these
add_compile_options(
    "/wd4251"
    "/wd4267"
)

add_executable(${PROJECT_NAME} 
   ${SRC_FILES} ${HEADERS_FILES}
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME "Sequentity"
    VERSION "0.1"
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(${PROJECT_NAME} PRIVATE)
    target_compile_options(${PROJECT_NAME} PRIVATE /std:c++17 /MDd /Zi /EHsc)
else()
    target_compile_definitions(${PROJECT_NAME} PRIVATE -DNDEBUG)
    target_compile_options(${PROJECT_NAME} PRIVATE /std:c++17 /EHsc /Od /wd4251 /MD)
endif()

# Magnum suffixes libraries for debug builds
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(MAGNUM_LIB_SUFFIX "-d")
else()
    set(MAGNUM_LIB_SUFFIX "")
endif()

target_link_libraries(${PROJECT_NAME}
    CorradePluginManager${MAGNUM_LIB_SUFFIX}
    CorradeUtility${MAGNUM_LIB_SUFFIX}
    glfw3
    opengl32
    Magnum${MAGNUM_LIB_SUFFIX}
    MagnumGL${MAGNUM_LIB_SUFFIX}
    MagnumMeshTools${MAGNUM_LIB_SUFFIX}
    MagnumPrimitives${MAGNUM_LIB_SUFFIX}
    MagnumGlfwApplication${MAGNUM_LIB_SUFFIX}
    MagnumShaders${MAGNUM_LIB_SUFFIX}
    MagnumTrade${MAGNUM_LIB_SUFFIX}
    MagnumImGuiIntegration${MAGNUM_LIB_SUFFIX}
)
