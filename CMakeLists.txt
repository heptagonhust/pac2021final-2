cmake_minimum_required(VERSION 3.2)

set(PROJECT_NAME ST_BarcodeMap-0.0.1)
project(${PROJECT_NAME})

set(CMAKE_CXX_STANDARD 17)
add_subdirectory(src/igzip)
find_library(ZSTD_LIB zstd REQUIRED)
find_path(ZSTD_INC zstd.h REQUIRED)
message("zstd library found in ${ZSTD_LIB}")
message("zstd headers found in ${ZSTD_INC}")

find_package(ZLIB)
find_package(Threads REQUIRED)
find_package(HDF5 REQUIRED)
find_package(Boost 1.70 REQUIRED COMPONENTS serialization)

add_definitions("-g" "-O3" "-DZWRAP_USE_ZSTD" "-gdwarf-4")
include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${ZSTD_INC})
include_directories(third_party/parallel-hashmap)
file(GLOB SRC_FILES ${PROJECT_SOURCE_DIR}/src/*.cpp)
file(GLOB ZSTD_FILES ${PROJECT_SOURCE_DIR}/src/zstd/*.c)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR})

add_executable(${PROJECT_NAME} ${SRC_FILES} ${ZSTD_FILES})
target_link_libraries(
  ${PROJECT_NAME}
  Boost::serialization
  HDF5::HDF5
  Threads::Threads
  igzipwrap
  ZLIB::ZLIB
  ${ZSTD_LIB}
)