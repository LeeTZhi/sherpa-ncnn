#include "c_api.h"
#include "asr_api.h"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>

#include "sherpa-ncnn/csrc/display.h"
#include "sherpa-ncnn/csrc/model.h"
#include "sherpa-ncnn/csrc/recognizer.h"

///model weights header files

#include "model/encoder_mem_larger.h"

#include "model/decoder_mem_larger.h"

#include "model/jointer_mem_larger.h"
#include "model/tokens_file_larger.h"

#define C_EXTERN extern "C"

class ASRRecognizer_Impl {
    public:
        ASRRecognizer_Impl() {
            recognizer_ = nullptr;
            stream_ = nullptr;
        }
        ~ASRRecognizer_Impl() {
            if (recognizer_ != nullptr) {
                delete recognizer_;
                recognizer_ = nullptr;
            }
            if (stream_ != nullptr) {
                delete stream_;
                stream_ = nullptr;
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
    protectd:
        SherpaNcnnRecognizer* recognizer_;
        SherpaNcnnStream* stream_;

        SherpaNcnnRecognizerConfig config_;
};

static void set_default_sherpa_ncnn_config(SherpaNcnnRecognizerConfig& config) {
    //Feature config
    config.feat_config.sample_rate = 16000.0f; //16kHz
    config.feat_config.feature_dim = 80;

    ///model config
    memset(&config.model_config, 0, sizeof(config.model_config));
    config.model_config.use_buffer = true;
    config.model_config.num_threads = 4;

    //decoder config
    config.decoder.decoder_config.num_active_paths = 1;
    config.decoder.decoder_config.decoding_method = nullptr;

    /* 
     for these parameters 
     1. endpoint and vad 
     2. hotwords
    */

    config.enable_endpoint = 1;
    config.endpoint_config.rule1_min_trailing_silence = 0.5f;
    config.endpoint_config.rule2_min_trailing_silence = 0.5f;
    config.endpoint_config.rule3_min_utterance_length = 0.5f;
    config.hotwords_file = nullptr;
    config.hotwords_score = 1.5f;
}

int ASRRecognizer_Impl::Init(const ASR_Parameters& asr_config ) {
    // model_config, config_ and recognizer_ are defined in recognizer.h
    set_default_sherpa_ncnn_config(config_);

    config_.model_config.encoder_param_buffer = encoder_jit_trace_pnnx_ncnn_param_bin;
    config_.model_config.encoder_bin_buffer = encoder_jit_trace_pnnx_ncnn_bin;
    config_.model_config.decoder_param_buffer = decoder_jit_trace_pnnx_ncnn_param_bin;
    config_.model_config.decoder_bin_buffer = decoder_jit_trace_pnnx_ncnn_bin;
    config_.model_config.jointer_param_buffer = jointer_jit_trace_pnnx_ncnn_param_bin;
    config_.model_config.jointer_bin_buffer = jointer_jit_trace_pnnx_ncnn_bin;
    config_.model_config.tokens_buffer = tokens_data;
    config_.model_config.tokens_buffer_size = sizeof(tokens_data);
    
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
    int ret = AcceptWaveform(stream_, samplerate, &audio_data_float[0], audioDataLen);

    ///if ready decode 
    ret = IsReady( recognizer_, stream_);
    if ( ret ) {
        ///decode
        ret = Decode(recognizer_, stream_);

        auto results = GetResult(recognizer_, stream_);

        ///asign the result to outputs
        result->text = results.text;
        //destroy the results
        DestroyResults(results);

        /// check endpoint
        *isEndPoint = IsEndPoint(recognizer_, stream_);
        if (*isEndPoint) {
            ///reset
            Reset(recognizer_, stream_);
        }
    }

    ///if is the final 
    if (isFinalStream) {
        ///reset
        InputFinished(stream_);
    }
    return 0;
}

C_EXTERN void* CreateStreamASRObject(const ASR_Parameters* asr_config) {
    ASRRecognizer_Impl* asr_recognizer = new ASRRecognizer_Impl();
    if (asr_recognizer->Init(*asr_config) != 0) {
        delete asr_recognizer;
        return nullptr;
    }
    return (void*)asr_recognizer;
}

C_EXTERN void DestroyStreamASRObject(void* asr_object) {
    if (asr_object == nullptr) {
        return;
    }
    ASRRecognizer_Impl* asr_recognizer = (ASRRecognizer_Impl*)asr_object;
    delete asr_recognizer;
}

C_EXTERN int ResetStreamASRObject(void* asr_object) {
    if (asr_object == nullptr) {
        return -1;
    }
    ASRRecognizer_Impl* asr_recognizer = (ASRRecognizer_Impl*)asr_object;
    Reset(asr_recognizer->recoginzer_, asr_recognizer->stream_);

    return 0;
}

C_EXTERN int StreamRecognize(
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