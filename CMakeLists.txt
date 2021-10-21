cmake_minimum_required(VERSION 3.2)

set(PROJECT_NAME ST_BarcodeMap-0.0.1)
project(${PROJECT_NAME})

set(CMAKE_CXX_STANDARD 17)
find_library(ISAL_LIB isal REQUIRED)
find_path(IGZIP_INC isa-l/igzip_lib.h REQUIRED)
message("isa-l library found in ${ISAL_LIB}")
message("igzip headers found in ${IGZIP_INC}")

find_package(HDF5 REQUIRED)
find_package(Boost 1.70 REQUIRED COMPONENTS serialization thread)

add_definitions(-O3 -march=icelake-server)
include_directories(third_party/parallel-hashmap)
file(GLOB SRC_FILES ${PROJECT_SOURCE_DIR}/src/*.cpp)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR})

add_executable(${PROJECT_NAME} ${SRC_FILES})
target_link_libraries(
  ${PROJECT_NAME}
  Boost::serialization
  Boost::thread
  hdf5
  pthread
  ${ISAL_LIB}
)