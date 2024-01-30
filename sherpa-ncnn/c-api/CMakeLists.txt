include_directories(${CMAKE_SOURCE_DIR})

configure_file(version.h.in version.h)

add_library(sherpa-ncnn-c-api c-api.cc)
target_link_libraries(sherpa-ncnn-c-api sherpa-ncnn-core)

if(BUILD_SHARED_LIBS)
  target_compile_definitions(sherpa-ncnn-c-api PRIVATE SHERPA_NCNN_BUILD_SHARED_LIBS=1)
  target_compile_definitions(sherpa-ncnn-c-api PRIVATE SHERPA_NCNN_BUILD_MAIN_LIB=1)
endif()

install(TARGETS sherpa-ncnn-c-api DESTINATION lib)

install(FILES c-api.h
  DESTINATION include/sherpa-ncnn/c-api
)

# ASR_API
include_directories(${CMAKE_SOURCE_DIR}/external/mbedtls/include)

add_library(asr-api SHARED asr_api_impl.cc c-api-internal.cc get_hardware_info.cc)
target_link_libraries(asr-api sherpa-ncnn-core get_cpu_info mbedtls mbedcrypto mbedx509)
target_include_directories(asr-api PRIVATE ${CMAKE_BINARY_DIR}/sherpa-ncnn/c-api)

install(TARGETS asr-api DESTINATION lib)
install(FILES asr_api.h DESTINATION include/sherpa-ncnn/c-api)
