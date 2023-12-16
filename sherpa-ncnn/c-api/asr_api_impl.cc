
#include "asr_api.h"
#include "c-api-internal.h"
#include <algorithm>
#include <memory>
#include <string>
#include <utility>

#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <cassert>
#include <stdlib.h>
#include <vector>

#include "tokens_file_larger.h"
///model weights header files

//typedef struct SherpaNcnnRecognizer SherpaNcnnRecognizer;
//typedef struct SherpaNcnnStream SherpaNcnnStream;
//typedef struct SherpaNcnnRecognizerConfig SherpaNcnnRecognizerConfig;

#define NULL_PTR_2_STR(t) (t != nullptr) ? t : ""

using namespace asr_api;

class ASRRecognizer_Impl {
    public:
        ASRRecognizer_Impl() {
            recognizer_ = nullptr;
            stream_ = nullptr;
            encoder_param_buffer_ = nullptr;
            encoder_bin_buffer_ = nullptr;
            decoder_param_buffer_ = nullptr;
            decoder_bin_buffer_ = nullptr;
            joiner_param_buffer_ = nullptr;
            joiner_bin_buffer_ = nullptr;
        }
        ~ASRRecognizer_Impl() {
            if (recognizer_ != nullptr) {
                DestroyRecognizer(recognizer_);
                recognizer_ = nullptr;
            }
            if (stream_ != nullptr) {
                DestroyStream(stream_);
                stream_ = nullptr;
            }
            if (encoder_param_buffer_ != nullptr) {
                free(encoder_param_buffer_);
                encoder_param_buffer_ = nullptr;
            }
            if (encoder_bin_buffer_ != nullptr) {
                free(encoder_bin_buffer_);
                encoder_bin_buffer_ = nullptr;
            }
            if (decoder_param_buffer_ != nullptr) {
                free(decoder_param_buffer_);
                decoder_param_buffer_ = nullptr;
            }
            if (decoder_bin_buffer_ != nullptr) {
                free(decoder_bin_buffer_);
                decoder_bin_buffer_ = nullptr;
            }
            if (joiner_param_buffer_ != nullptr) {
                free(joiner_param_buffer_);
                joiner_param_buffer_ = nullptr;
            }
            if (joiner_bin_buffer_ != nullptr) {
                free(joiner_bin_buffer_);
                joiner_bin_buffer_ = nullptr;
            }
        }

        /// @brief initialize the recognizer and stream
        /// @return Success or error code
        int Init(const ASR_Parameters& asr_config );

        ///stream recognize
        ///@param audioData: audio data
        ///@param audioDataLen: audio data length
        ///@param isFinalStream: whether is the final streams, if true, the recoginze will be end and reset
        ///@param result: ASR_Result
        ///@param isEndPoint: is end point
        ///If Success, return 0, else return other error code
        int StreamRecognize(
            const int16_t* audioData, 
            int audioDataLen, 
            int isFinalStream,
            float sampleRate,
            ASR_Result* result, 
            int* isEndPoint
            );
        int Reset() {
            if ( recognizer_ != nullptr && stream_ != nullptr) {
                ::Reset(recognizer_, stream_);
                return 0;
            }
            else {
                return -1;
            }
        }
    protected:
        SherpaNcnnRecognizer* recognizer_;
        SherpaNcnnStream* stream_;

        SherpaNcnnRecognizerConfig config_;

        ///six model weights
        uint8_t* encoder_param_buffer_;
        uint8_t* encoder_bin_buffer_;
        uint8_t* decoder_param_buffer_;
        uint8_t* decoder_bin_buffer_;
        uint8_t* joiner_param_buffer_;
        uint8_t* joiner_bin_buffer_;
};

static bool load_from_merged_file(
    const std::string& merged_file_name, 
    uint8_t** encoder_param_buffer, 
    uint8_t** encoder_bin_buffer,
    uint8_t** decoder_param_buffer,
    uint8_t** decoder_bin_buffer,
    uint8_t** joiner_param_buffer,
    uint8_t** joiner_bin_buffer);

static void set_default_sherpa_ncnn_config(SherpaNcnnRecognizerConfig& config) {
    //Feature config
    config.feat_config.sampling_rate = 16000.0f; //16kHz
    config.feat_config.feature_dim = 80;

    ///model config
    memset(&config.model_config, 0, sizeof(config.model_config));
    config.model_config.buffer_flag = 1;
    config.model_config.num_threads = 4;

    //decoder config
    config.decoder_config.num_active_paths = 1;
    config.decoder_config.decoding_method = nullptr;

    /* 
     for these parameters 
     1. endpoint and vad 
     2. hotwords
    */

    config.decoder_config.num_active_paths = 4;
    config.enable_endpoint = 1;
    config.rule1_min_trailing_silence = 2.4;
    config.rule2_min_trailing_silence = 1.2;
    config.rule3_min_utterance_length = 300;

    config.hotwords_file = nullptr;
    config.hotwords_score = 1.5f;
}

int ASRRecognizer_Impl::Init(const ASR_Parameters& asr_config ) {
    // model_config, config_ and recognizer_ are defined in recognizer.h
    set_default_sherpa_ncnn_config(config_);
    std::string model_name;
    ///load the model weights
    if (asr_config.version == FAST ) {
        model_name = NULL_PTR_2_STR(asr_config.faster_model_name);
    }
    else {
        model_name = NULL_PTR_2_STR(asr_config.larger_model_name);
    }
    /// if model name is empty, return error
    if (model_name.empty()) {
        return -1;
    }
    ///load the model weights
    bool bLoad = load_from_merged_file(
            model_name, 
            &encoder_param_buffer_, 
            &encoder_bin_buffer_,
            &decoder_param_buffer_,
            &decoder_bin_buffer_,
            &joiner_param_buffer_,
            &joiner_bin_buffer_
            );
    if (!bLoad) {
        return -1;
    }

    config_.model_config.encoder_param_buffer = encoder_param_buffer_;
    config_.model_config.encoder_bin_buffer = encoder_bin_buffer_;
    config_.model_config.decoder_param_buffer = decoder_param_buffer_;
    config_.model_config.decoder_bin_buffer = decoder_bin_buffer_;
    config_.model_config.joiner_param_buffer = joiner_param_buffer_;
    config_.model_config.joiner_bin_buffer = joiner_bin_buffer_;

    config_.model_config.tokens_buffer = reinterpret_cast<const unsigned char*>(tokens_data);
    config_.model_config.tokens_buffer_size = sizeof(tokens_data);
    
    ///endpoint parameters
    config_.enable_endpoint = asr_config.enable_endpoint;
    config_.rule1_min_trailing_silence = asr_config.rule1_min_threshold;
    config_.rule2_min_trailing_silence = asr_config.rule2_min_threshold;
    config_.rule3_min_utterance_length = asr_config.rule3_min_threshold;
    ///hotwords
    config_.hotwords_file = asr_config.hotwords_path;
    config_.hotwords_score = asr_config.hotwords_factor;
    
    recognizer_ = CreateRecognizer(&config_);

    stream_ = CreateStream(recognizer_);
    return 0;
}

int ASRRecognizer_Impl::StreamRecognize(
    const int16_t* audioData, 
    int audioDataLen, 
    int isFinalStream,
    float sampleRate,
    ASR_Result* result, 
    int* isEndPoint
    ) {
    if (audioData == nullptr || audioDataLen <= 0) {
        return -1;
    }
    if (result == nullptr) {
        return -1;
    }
    if (isEndPoint == nullptr) {
        return -1;
    }

    ///convert the audio data to float
    std::vector<float> audio_data_float(audioDataLen);
    for (int i = 0; i < audioDataLen; i++) {
        audio_data_float[i] = audioData[i]/32768.0f;
    }
    /// accept wave data
    AcceptWaveform(stream_, sampleRate, &audio_data_float[0], audioDataLen);

    ///if ready decode 
    //int ret = IsReady( recognizer_, stream_);
    while ( IsReady( recognizer_, stream_) ) {
        ///decode
        Decode(recognizer_, stream_);

        auto results = GetResult(recognizer_, stream_);
        if (results->text) {
            ///copy the result to outputs
            result->text = strdup(results->text);
        }
        ///asign the result to outputs
        //result->text = results->text;
        //destroy the results
        DestroyResult(results);

        /// check endpoint
        *isEndPoint = ::IsEndpoint(recognizer_, stream_);
        if (*isEndPoint) {
            ///reset
            this->Reset();
        }
    }

    ///if is the final 
    if (isFinalStream) {
        ///reset
        InputFinished(stream_);
    }
    return 0;
}

extern "C" {

ASR_API_EXPORT void* CreateStreamASRObject(
    const ASR_Parameters* asr_config, 
    const char* authToken,
    const int authTokenLen
    ) {
    ASRRecognizer_Impl* asr_recognizer = new ASRRecognizer_Impl();
    if (asr_recognizer->Init(*asr_config) != 0) {
        delete asr_recognizer;
        return nullptr;
    }
    return (void*)asr_recognizer;
}

ASR_API_EXPORT  void DestroyStreamASR(void* asr_object) {
    if (asr_object == nullptr) {
        return;
    }
    ASRRecognizer_Impl* asr_recognizer = (ASRRecognizer_Impl*)asr_object;
    delete asr_recognizer;
}

ASR_API_EXPORT  int ResetStreamASR(void* asr_object) {
    if (asr_object == nullptr) {
        return -1;
    }
    ASRRecognizer_Impl* asr_recognizer = (ASRRecognizer_Impl*)asr_object;
    return asr_recognizer->Reset();
}

ASR_API_EXPORT  int StreamRecognize(
    void* streamASR, 
    const int16_t* audioData, 
    int audioDataLen, 
    float sampleRate,
    int isFinalStream,
    ASR_Result* result, 
    int* isEndPoint) {
    
    if (streamASR == nullptr) {
        return -1;
    }

    ASRRecognizer_Impl* asr_recognizer = (ASRRecognizer_Impl*)streamASR;
    return asr_recognizer->StreamRecognize(audioData, audioDataLen, isFinalStream, sampleRate, result, isEndPoint);
}

ASR_API_EXPORT int DestroyASRResult(ASR_Result* result){
    ///TODO
    if ( result == nullptr) {
        return 0;
    }
    free(result->text);
    result->text = nullptr;
    return 0;
}

} //extern "C"

//1 byte alignment
#pragma pack(1)
typedef struct file_header {
  char file_id[2];
  uint32_t file_size;
} file_header;
//end 1 byte alignment
#pragma pack()

static void process_buffer_with_magic_number(uint8_t* buffer, uint32_t buffer_size, uint32_t magic_number) {
    for (int i = 0; i < buffer_size; ++i) {
        buffer[i] ^= (magic_number >> ((i % 4) * 8)) & 0xFF;
    }
}

#define SURE_NEW(p) {if (p == nullptr) {std::cout << "Memory allocation failed at: " << __LINE__ <<std::endl; return false;}}

#define SURE_READ(is, cnt) do { \
    if (is) \
      std::cout << "all characters read successfully."<<std::endl; \
    else    \
      std::cout << "read "<< (cnt) <<" but error: only " << is.gcount() << " could be read"<<std::endl; \
} while(0)
#define ALIGN_SIZE(size, alignment) (((size) + (alignment) - 1) / (alignment) * (alignment))

static uint8_t* _aligned_alloc(size_t alignment, size_t size) {
    void* ptr = nullptr;
    size = ALIGN_SIZE(size, alignment);
    int ret = posix_memalign(&ptr, alignment, size);
    if (ret != 0) {
        std::cout << "Failed to allocate memory with alignment: " << alignment << ", size: " << size << std::endl;
        return nullptr;
    }
    return static_cast<uint8_t*>(ptr);
}

/*
Load six buffers from the merged file, memory allocate in this function
@param merged_file_name: merged file name
@param encoder_param_buffer: encoder param buffer
@param encoder_bin_buffer: encoder bin buffer
@param decoder_param_buffer: decoder param buffer
@param decoder_bin_buffer: decoder bin buffer
@param joint_param_buffer: joint param buffer
@param joint_bin_buffer: joint bin buffer
if load successfully, return true, otherwise return false
*/
bool load_from_merged_file(
    const std::string& merged_file_name, 
    uint8_t** encoder_param_buffer, 
    uint8_t** encoder_bin_buffer,
    uint8_t** decoder_param_buffer,
    uint8_t** decoder_bin_buffer,
    uint8_t** joiner_param_buffer,
    uint8_t** joiner_bin_buffer) {
    
    ///set all point to nullptr
    *encoder_param_buffer = nullptr;
    *encoder_bin_buffer = nullptr;
    *decoder_param_buffer = nullptr;
    *decoder_bin_buffer = nullptr;
    *joiner_param_buffer = nullptr;
    *joiner_bin_buffer = nullptr;

    size_t aligned_size = 8;

    std::ifstream merged_file(merged_file_name.c_str(), std::ios::binary);
    if (!merged_file) {
        std::cout << "Failed to open file: " << merged_file_name << std::endl;
        return false;
    }
    uint32_t magic_number = 0;
    merged_file.read((char*)&magic_number, sizeof(magic_number));
    SURE_READ(merged_file, sizeof(magic_number));
    
    //read EP file header and data
    file_header fh;
    merged_file.read((char*)&fh, sizeof(fh));
    SURE_READ(merged_file, sizeof(fh));
    assert(memcmp(fh.file_id, "EP", 2) == 0);
    *encoder_param_buffer = _aligned_alloc(aligned_size, fh.file_size);
    SURE_NEW(*encoder_param_buffer);

    //*encoder_param_buffer = static_cast<uint8_t*>(aligned_alloc(4, fh.file_size));
    //SURE_NEW(*encoder_param_buffer);
    merged_file.read((char*)*encoder_param_buffer, fh.file_size);
    SURE_READ(merged_file, fh.file_size);
    ///magic number xor
    process_buffer_with_magic_number(*encoder_param_buffer, fh.file_size, magic_number);

    //read EB file header and data
    merged_file.read((char*)&fh, sizeof(fh));
    SURE_READ(merged_file, sizeof(fh));
    assert(memcmp(fh.file_id, "EB", 2) == 0);
    
    *encoder_bin_buffer = _aligned_alloc(aligned_size, fh.file_size);
    SURE_NEW(*encoder_bin_buffer);
    merged_file.read((char*)*encoder_bin_buffer, fh.file_size);
    SURE_READ(merged_file, fh.file_size);
    ///magic number xor
    process_buffer_with_magic_number(*encoder_bin_buffer, fh.file_size, magic_number);

    //read DP file header and data
    merged_file.read((char*)&fh, sizeof(fh));
    SURE_READ(merged_file, sizeof(fh));
    assert(memcmp(fh.file_id, "DP", 2) == 0);
    *decoder_param_buffer = _aligned_alloc(aligned_size, fh.file_size);;
    SURE_NEW(*decoder_param_buffer);
    merged_file.read((char*)*decoder_param_buffer, fh.file_size);
    SURE_READ(merged_file, fh.file_size);
    ///magic number xor
    process_buffer_with_magic_number(*decoder_param_buffer, fh.file_size, magic_number);

    //read DB file header and data
    merged_file.read((char*)&fh, sizeof(fh));
    SURE_READ(merged_file, sizeof(fh));
    assert(memcmp(fh.file_id, "DB", 2) == 0);
    *decoder_bin_buffer = _aligned_alloc(aligned_size, fh.file_size);
    SURE_NEW(*decoder_bin_buffer);
    merged_file.read((char*)*decoder_bin_buffer, fh.file_size);
    SURE_READ(merged_file, fh.file_size);
    ///magic number xor
    process_buffer_with_magic_number(*decoder_bin_buffer, fh.file_size, magic_number);

    //read JP file header and data
    merged_file.read((char*)&fh, sizeof(fh));
    SURE_READ(merged_file, sizeof(fh));
    assert(memcmp(fh.file_id, "JP", 2) == 0);
    *joiner_param_buffer = _aligned_alloc(aligned_size, fh.file_size);
    SURE_NEW(*joiner_param_buffer);
    merged_file.read((char*)*joiner_param_buffer, fh.file_size);
    SURE_READ(merged_file, fh.file_size);
    ///magic number xor
    process_buffer_with_magic_number(*joiner_param_buffer, fh.file_size, magic_number);

    //read JB file header and data
    merged_file.read((char*)&fh, sizeof(fh));
    SURE_READ(merged_file, sizeof(fh));
    assert(memcmp(fh.file_id, "JB", 2) == 0);
    *joiner_bin_buffer = _aligned_alloc(aligned_size, fh.file_size);
    SURE_NEW(*joiner_bin_buffer);
    merged_file.read((char*)*joiner_bin_buffer, fh.file_size);
    SURE_READ(merged_file, fh.file_size);
    merged_file.close();
    ///magic number xor
    process_buffer_with_magic_number(*joiner_bin_buffer, fh.file_size, magic_number);

    return true;
}