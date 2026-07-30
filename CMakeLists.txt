cmake_minimum_required(VERSION 3.28)

project(dds-intercom VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(miniaudio REQUIRED IMPORTED_TARGET miniaudio)
find_package(fastdds REQUIRED)
find_package(CLI11 REQUIRED)

add_subdirectory(schema)

add_executable(${PROJECT_NAME})

target_compile_options(${PROJECT_NAME} PRIVATE
  -Wall -Wextra -Wpedantic -Wconversion -Wshadow
)

target_sources(${PROJECT_NAME}
  PRIVATE src/main.cpp
  PRIVATE FILE_SET cxx_modules TYPE CXX_MODULES FILES
    src/atomic_ring_buffer.cppm
    src/dds_node.cppm
    src/intercom_engine.cppm
)

target_link_libraries(${PROJECT_NAME} PRIVATE
  PkgConfig::miniaudio
  fastdds
  schema_lib
  CLI11::CLI11
)

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)

find_program(CLANG_FORMAT clang-format)
if(CLANG_FORMAT)
  file(GLOB_RECURSE ALL_CXX_FILES
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/*.cppm"
  )
  add_custom_target(format
    COMMAND ${CLANG_FORMAT} -i --verbose ${ALL_CXX_FILES}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Formatting source files with clang-format"
    USES_TERMINAL
  )
endif()

find_program(CLANG_TIDY clang-tidy)
if(CLANG_TIDY)
  file(GLOB_RECURSE ALL_CXX_FILES
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/*.cppm"
  )
  add_custom_target(tidy
    COMMAND ${CLANG_TIDY} -p ${CMAKE_BINARY_DIR} ${ALL_CXX_FILES}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Linting source files with clang-tidy"
    USES_TERMINAL
  )
endif()
