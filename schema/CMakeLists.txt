add_custom_command(
  OUTPUT
    AudioFrame.hpp
    AudioFramePubSubTypes.cxx
    AudioFramePubSubTypes.hpp
    AudioFrameTypeObjectSupport.cxx
    AudioFrameTypeObjectSupport.hpp
  COMMAND fastddsgen -replace -d ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/AudioFrame.idl
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/AudioFrame.idl
  COMMENT "Running fastddsgen on AudioFrame.idl"
  VERBATIM
)

add_library(schema_lib STATIC
  AudioFramePubSubTypes.cxx
  AudioFrameTypeObjectSupport.cxx
)

target_include_directories(schema_lib PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(schema_lib PUBLIC fastdds)
