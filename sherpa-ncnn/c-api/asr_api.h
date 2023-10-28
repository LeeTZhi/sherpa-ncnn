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
    const char* larger_model_name;
    const char* faster_model_name;
    char reserved[256];
} ASR_Parameters;

typedef struct ASR_Result
{
    int32_t size;
    int32_t result_size;
    int32_t result_capacity;
    char* text;
    int32_t reserved[256];
} ASR_Result;

///API

ASR_API_EXPORT void* CreateStreamASRObject(
    const ASR_Parameters* parameters,
    const char* authToken,
    const int authTokenLen
    );

/* 
    * DestroyStreamASRObject
    * @param streamASR: void*
    * @return: void
*/
ASR_API_EXPORT void DestroyStreamASRObject(void* streamASR);

/*
 * Streaming recognize, if isEndPoint is true, the results will be stored in result, after get the results, 
 * you should call DestroyASRResult to free the memory
    * @param streamASR: void*  
    * @param audioData: audio data
    * @param audioDataLen: audio data length
    * @param isFinalStream: whether is the final streams, if true, the recoginze will be end and reset
    * @param result: ASR_Result
    * @param isEndPoint: is end point
    * If Success, return 0, else return other error code   
*/
ASR_API_EXPORT int StreamRecognize(
    void* streamASR, 
    const int16_t* audioData, 
    int audioDataLen, 
    float sampleRate,
    int isFinalStream,
    ASR_Result* result, 
    int* isEndPoint
    );

/*
    * release the memory of ASR_Result
    * @param result: ASR_Result
    * If Success, return 0, else return other error code   
*/
ASR_API_EXPORT int DestroyASRResult(ASR_Result* result);

/* reset the stream recognize
    * @param streamASR: void*
    * @return: If Success, return 0, else return other error code   
*/
ASR_API_EXPORT int ResetStreamASR(void* streamASR);


#ifdef __cplusplus
}
#endif 

#ifdef __cplusplus
class ASRClass {
public:
    ASRClass() {
        streamASRObject = nullptr;
    }

    int Init(const ASR_Parameters* parameters, const char* authToken, const int authTokenLen) {
        

        streamASRObject = CreateStreamASRObject(parameters, authToken, authTokenLen);
        if ( streamASRObject == nullptr ) {
            return -1;
        }
    }

    ~ASRClass() {
        if ( streamASRObject != nullptr ) {
            DestroyStreamASRObject(streamASRObject);
            streamASRObject = nullptr;
        }
    }

    int StreamRecognize_(
        const int16_t* audioData,
        const int audioDataLen,
        int isFinalStream,
        float sampleRate,
        ASR_Result* result, 
        int* isEndPoint
        ) {
        return ::StreamRecognize(streamASRObject, audioData, audioDataLen, sampleRate, isFinalStream, result, isEndPoint);
    }

    int ResetStreamASR_() {
        return ::ResetStreamASR(streamASRObject);
    }

protected:
    void* asrObject;
    void* streamASRObject;
};
#endif

#endif // ASR_API_H