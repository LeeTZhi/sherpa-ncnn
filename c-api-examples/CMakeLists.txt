include_directories(${CMAKE_SOURCE_DIR})
add_executable(decode-file-c-api decode-file-c-api.c)
target_link_libraries(decode-file-c-api sherpa-ncnn-c-api)

add_executable(decode-file-asr decode-file-c-asr-api.c)
target_link_libraries(decode-file-asr asr-api)