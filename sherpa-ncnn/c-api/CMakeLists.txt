include_directories(${CMAKE_SOURCE_DIR})
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
add_library(asr-api asr_api_impl.cc c-api-internal.cc)
target_link_libraries(asr-api sherpa-ncnn-core)
install(TARGETS asr-api DESTINATION lib)
install(FILES asr-api.h DESTINATION include/sherpa-ncnn/c-api)
