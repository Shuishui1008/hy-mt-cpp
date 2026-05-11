cmake_minimum_required(VERSION 3.18)
project(hy-mt-cpp LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Options
option(HYMT_BUILD_CLI   "Build command-line apps"             ON)
option(HYMT_BUILD_C_API "Build C API shared lib for bindings" ON)
option(HYMT_USE_METAL   "Use Metal backend (macOS/iOS)"       OFF)
option(HYMT_USE_CUDA    "Use CUDA backend"                    OFF)
option(HYMT_USE_VULKAN  "Use Vulkan backend"                  OFF)
option(HYMT_USE_OPENMP  "Enable OpenMP in llama.cpp"          ON)

set(LLAMA_CPP_TAG "master" CACHE STRING "llama.cpp git tag/branch to fetch")

# Pull llama.cpp — either from local path or via FetchContent
set(LLAMA_CPP_LOCAL "" CACHE PATH "Path to a local llama.cpp source tree (skips FetchContent)")

set(LLAMA_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(LLAMA_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(LLAMA_BUILD_SERVER   OFF CACHE BOOL "" FORCE)
set(LLAMA_CURL           OFF CACHE BOOL "" FORCE)

if(HYMT_USE_METAL)
  set(GGML_METAL ON CACHE BOOL "" FORCE)
else()
  set(GGML_METAL OFF CACHE BOOL "" FORCE)
endif()
if(HYMT_USE_CUDA)
  set(GGML_CUDA ON CACHE BOOL "" FORCE)
endif()
if(HYMT_USE_VULKAN)
  set(GGML_VULKAN ON CACHE BOOL "" FORCE)
endif()
if(HYMT_USE_OPENMP)
  set(GGML_OPENMP ON CACHE BOOL "" FORCE)
else()
  set(GGML_OPENMP OFF CACHE BOOL "" FORCE)
endif()

if(LLAMA_CPP_LOCAL AND EXISTS "${LLAMA_CPP_LOCAL}/CMakeLists.txt")
  message(STATUS "Using local llama.cpp: ${LLAMA_CPP_LOCAL}")
  add_subdirectory("${LLAMA_CPP_LOCAL}" "${CMAKE_CURRENT_BINARY_DIR}/llama_cpp")
else()
  include(FetchContent)
  FetchContent_Declare(
    llama_cpp
    GIT_REPOSITORY https://github.com/ggml-org/llama.cpp.git
    GIT_TAG        ${LLAMA_CPP_TAG}
    GIT_SHALLOW    TRUE
  )
  FetchContent_MakeAvailable(llama_cpp)
endif()

# Core library
add_library(hymt STATIC
  src/translator.cc
  src/prompt.cc
)
target_include_directories(hymt PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
  $<INSTALL_INTERFACE:include>
)
target_link_libraries(hymt PUBLIC llama)

# C API shared library
if(HYMT_BUILD_C_API)
  add_library(hymt_c SHARED src/c_api.cc)
  target_link_libraries(hymt_c PUBLIC hymt)
  set_target_properties(hymt_c PROPERTIES OUTPUT_NAME "hymt" VERSION "0.1.0" SOVERSION "0")
endif()

# CLI apps
if(HYMT_BUILD_CLI)
  add_executable(hymt_cli apps/hymt_cli.cc)
  target_link_libraries(hymt_cli PRIVATE hymt)

  add_executable(hymt_repl apps/hymt_repl.cc)
  target_link_libraries(hymt_repl PRIVATE hymt)
endif()

message(STATUS "hy-mt-cpp: llama.cpp=${LLAMA_CPP_TAG} Metal=${HYMT_USE_METAL} CUDA=${HYMT_USE_CUDA} Vulkan=${HYMT_USE_VULKAN} OpenMP=${HYMT_USE_OPENMP}")
