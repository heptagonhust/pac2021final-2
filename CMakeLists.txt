cmake_minimum_required(VERSION 3.2)

set(PROJECT_NAME ST_BarcodeMap-0.0.1)
project(${PROJECT_NAME})

set(CMAKE_CXX_STANDARD 11)

find_package(ZLIB)
find_package(Threads REQUIRED)
find_package(HDF5 REQUIRED)
find_package(Boost 1.70 REQUIRED COMPONENTS serialization)

add_definitions("-g" "-O3")
include_directories(${PROJECT_SOURCE_DIR}/src)
file(GLOB SRC_FILES ${PROJECT_SOURCE_DIR}/src/*.cpp)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR})

add_executable(${PROJECT_NAME} ${SRC_FILES})
target_link_libraries(
  ${PROJECT_NAME}
  Boost::serialization
  HDF5::HDF5
  Threads::Threads
  ZLIB::ZLIB
)
