include_directories(${CMAKE_SOURCE_DIR})
add_executable(decode-file-c-api decode-file-c-api.c)
target_link_libraries(decode-file-c-api sherpa-ncnn-c-api)

add_executable(decode-file-asr decode-file-c-asr-api.c)
target_link_libraries(decode-file-asr asr-api m)

include_directories(${CMAKE_SOURCE_DIR}/sherpa-ncnn/c-api)
add_executable(recog_demo recog_demo.cpp)
target_link_libraries(recog_demo asr-api m)