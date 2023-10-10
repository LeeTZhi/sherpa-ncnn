#ifndef ASR_API_H
#define ASR_API_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// See https://github.com/pytorch/pytorch/blob/main/c10/macros/Export.h
// We will set SHERPA_NCNN_BUILD_SHARED_LIBS and SHERPA_NCNN_BUILD_MAIN_LIB in
// CMakeLists.txt

#if defined(_WIN32)
#if defined(ASR_API_BUILD_SHARED_LIBS)
#define ASR_API_EXPORT __declspec(dllexport)
#define ASR_API_IMPORT __declspec(dllimport)
#else
#define ASR_API_EXPORT
#define ASR_API_IMPORT
#endif
#else  // WIN32
//define the linux so export, visibility hidden 
#define ASR_API_EXPORT __attribute__((visibility("default")))
#define ASR_API_IMPORT ASR_API_EXPORT
#endif

#if defined(ASR_API_BUILD_MAIN_LIB)
#define ASR_API_API ASR_API_EXPORT
#else
#define ASR_API_API ASR_API_IMPORT
#endif

enum ASR_Version { FAST=0x01, ACCURATE=0x02 };

///struct 
typedef struct ASR_Parameters
{
    int32_t size;
    int version; //ASR_Version
    int recognize_mode;
    char reserved[256];
} ASR_Parameters;

///API
/* 
 * CreateASRObject
 * @param parameters: ASR_Parameters
 * @param authToken: auth token
 * @return: void*
 */
void* CreateASRObject(const ASR_Parameters* parameters, const char* authToken);

/* 
 * DestroyASRObject
 * @param asr: void*
 * @return: void
 */
void DestroyASRObject(void* asr);

/*
  Create Stream ASR Object 
    @param parameters: ASR_Parameters
    @param asrObject: void*
    @return: void*
*/
void* CreateStreamASRObject(const ASR_Parameters* parameters, void* asrObject);


#ifdef __cplusplus
}
#endif

#endif // ASR_API_H