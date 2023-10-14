#include "c_api.h"
#include "asr_api_impl.h"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>

#include "sherpa-ncnn/csrc/display.h"
#include "sherpa-ncnn/csrc/model.h"
#include "sherpa-ncnn/csrc/recognizer.h"

class ASRRecognizer_Impl {
    public:
        /// @brief initialize the recognizer and stream
        /// @return Success or error code
        int Init();

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
            ASR_Result* result, 
            int* isEndPoint
            );
    protectd:
        std::unique_ptr<sherpa_ncnn::Recognizer> recognizer_;
        std::unique_ptr<sherpa_ncnn::Stream> stream_;

        SherpaNcnnRecognizerConfig config_;
};