/**
 * Copyright (c)  2023  Xiaomi Corporation (authors: Fangjun Kuang)
 *
 * See LICENSE for clarification regarding multiple authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "sherpa-ncnn/c-api/c-api-internal.h"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>

#include "sherpa-ncnn/csrc/display.h"
#include "sherpa-ncnn/csrc/model.h"
#include "sherpa-ncnn/csrc/recognizer.h"

namespace asr_api {

struct SherpaNcnnRecognizer {
  std::unique_ptr<sherpa_ncnn::Recognizer> recognizer;
};

struct SherpaNcnnStream {
  std::unique_ptr<sherpa_ncnn::Stream> stream;
};

struct SherpaNcnnDisplay {
  std::unique_ptr<sherpa_ncnn::Display> impl;
};

#define SHERPA_NCNN_OR(x, y) (x ? x : y)
#define NULL_PTR_2_STR(t) (t != nullptr) ? t : ""

SherpaNcnnRecognizer *CreateRecognizer(
    const SherpaNcnnRecognizerConfig *in_config) {
  // model_config
  sherpa_ncnn::RecognizerConfig config;
  config.model_config.encoder_param = NULL_PTR_2_STR(in_config->model_config.encoder_param);
  config.model_config.encoder_bin = NULL_PTR_2_STR(in_config->model_config.encoder_bin);

  config.model_config.decoder_param = NULL_PTR_2_STR(in_config->model_config.decoder_param);
  config.model_config.decoder_bin = NULL_PTR_2_STR(in_config->model_config.decoder_bin);

  config.model_config.joiner_param = NULL_PTR_2_STR(in_config->model_config.joiner_param);
  config.model_config.joiner_bin = NULL_PTR_2_STR(in_config->model_config.joiner_bin);

  config.model_config.tokens = NULL_PTR_2_STR(in_config->model_config.tokens);
  config.model_config.use_vulkan_compute =
      in_config->model_config.use_vulkan_compute;

  int32_t num_threads = SHERPA_NCNN_OR(in_config->model_config.num_threads, 1);

  ////tokens config
  config.model_config.tokens_buf = in_config->model_config.tokens_buffer;
  config.model_config.tokens_buf_size = in_config->model_config.tokens_buffer_size;

  config.model_config.encoder_opt.num_threads = num_threads;
  config.model_config.decoder_opt.num_threads = num_threads;
  config.model_config.joiner_opt.num_threads = num_threads;

  #if defined(__aarch64__) 
  //set use a53 = True
  config.model_config.encoder_opt.use_a53_a55_optimized_kernel = true;
  config.model_config.decoder_opt.use_a53_a55_optimized_kernel = true;
  config.model_config.joiner_opt.use_a53_a55_optimized_kernel = true;
  config.model_config.encoder_opt.use_bf16_storage = false;
  config.model_config.decoder_opt.use_bf16_storage = false;
  config.model_config.joiner_opt.use_bf16_storage = false;

  //use_packing_layout
  config.model_config.encoder_opt.use_packing_layout = true;
  config.model_config.decoder_opt.use_packing_layout = true;
  config.model_config.joiner_opt.use_packing_layout = true;
  #endif 

  

  // decoder_config
  //NCNN_LOGE("in config decoder method: %s\n", in_config->decoder_config.decoding_method?in_config->decoder_config.decoding_method:"null");

  if ( in_config->decoder_config.decoding_method == nullptr ){
    config.decoder_config.method = "greedy_search";
    config.decoder_config.num_active_paths = 1;
  }
  else {
    config.decoder_config.method = in_config->decoder_config.decoding_method;
    config.decoder_config.num_active_paths =
      in_config->decoder_config.num_active_paths;
  }
  
  if (in_config->hotwords_file != nullptr) {
    //beam search
    config.decoder_config.method = "modified_beam_search"; //"modified_beam_search";
    config.decoder_config.num_active_paths = std::max(2, config.decoder_config.num_active_paths);
  }

  config.hotwords_file = SHERPA_NCNN_OR(in_config->hotwords_file, "");
  config.hotwords_score = std::max(in_config->hotwords_score, 1.5f);
  NCNN_LOGE("hotwords_file: %s, hotwords_score: %f", config.hotwords_file.c_str(), in_config->hotwords_score);
  //num_active_paths
  NCNN_LOGE("active_paths: %d", config.decoder_config.num_active_paths);

  config.enable_endpoint = in_config->enable_endpoint;

  config.endpoint_config.rule1.min_trailing_silence =
      in_config->rule1_min_trailing_silence;

  config.endpoint_config.rule2.min_trailing_silence =
      in_config->rule2_min_trailing_silence;

  config.endpoint_config.rule3.min_utterance_length =
      in_config->rule3_min_utterance_length;

  config.feat_config.sampling_rate =
      SHERPA_NCNN_OR(in_config->feat_config.sampling_rate, 16000);

  config.feat_config.feature_dim =
      SHERPA_NCNN_OR(in_config->feat_config.feature_dim, 80);

  ///ncnn model memory buffer
  config.model_config.encoder_param_buf =
      in_config->model_config.encoder_param_buffer;
  config.model_config.encoder_bin_buf = in_config->model_config.encoder_bin_buffer;
  config.model_config.decoder_param_buf =
      in_config->model_config.decoder_param_buffer;
  config.model_config.decoder_bin_buf = in_config->model_config.decoder_bin_buffer;
  config.model_config.joiner_param_buf =
      in_config->model_config.joiner_param_buffer;
  config.model_config.joiner_bin_buf = in_config->model_config.joiner_bin_buffer;
  config.model_config.tokens_buf = in_config->model_config.tokens_buffer;
  ///flag 
  config.model_config.use_buffer = static_cast<bool>(in_config->model_config.buffer_flag);

  auto recognizer = std::make_unique<sherpa_ncnn::Recognizer>(config);

  if (!recognizer->GetModel()) {
    NCNN_LOGE("Failed to create the recognizer! Please check your config: %s",
              config.ToString().c_str());
    return nullptr;
  }

  auto ans = new SherpaNcnnRecognizer;
  ans->recognizer = std::move(recognizer);
  return ans;
}

void DestroyRecognizer(SherpaNcnnRecognizer *p) { delete p; }

SherpaNcnnStream *CreateStream(SherpaNcnnRecognizer *p) {
  auto ans = new SherpaNcnnStream;
  ans->stream = p->recognizer->CreateStream();
  return ans;
}

void DestroyStream(SherpaNcnnStream *s) { delete s; }

void AcceptWaveform(SherpaNcnnStream *s, float sample_rate,
                    const float *samples, int32_t n) {
  s->stream->AcceptWaveform(sample_rate, samples, n);
}

int32_t IsReady(SherpaNcnnRecognizer *p, SherpaNcnnStream *s) {
  return p->recognizer->IsReady(s->stream.get());
}

void Decode(SherpaNcnnRecognizer *p, SherpaNcnnStream *s) {
  p->recognizer->DecodeStream(s->stream.get());
}

SherpaNcnnResult *GetResult(SherpaNcnnRecognizer *p, SherpaNcnnStream *s) {
  std::string text = p->recognizer->GetResult(s->stream.get()).text;
  auto res = p->recognizer->GetResult(s->stream.get());

  auto r = new SherpaNcnnResult;
  r->text = new char[text.size() + 1];
  std::copy(text.begin(), text.end(), const_cast<char *>(r->text));
  const_cast<char *>(r->text)[text.size()] = 0;
  r->count = res.tokens.size();
  if (r->count > 0) {
    // Each word ends with nullptr
    r->tokens = new char[text.size() + r->count];
    memset(reinterpret_cast<void *>(const_cast<char *>(r->tokens)), 0,
           text.size() + r->count);
    r->timestamps = new float[r->count];
    int pos = 0;
    for (int32_t i = 0; i < r->count; ++i) {
      memcpy(reinterpret_cast<void *>(const_cast<char *>(r->tokens + pos)),
             res.stokens[i].c_str(), res.stokens[i].size());
      pos += res.stokens[i].size() + 1;
      r->timestamps[i] = res.timestamps[i];
    }
  } else {
    r->timestamps = nullptr;
    r->tokens = nullptr;
  }

  return r;
}

void DestroyResult(const SherpaNcnnResult *r) {
  delete[] r->text;
  delete[] r->timestamps;  // it is ok to delete a nullptr
  delete[] r->tokens;
  delete r;
}

void Reset(SherpaNcnnRecognizer *p, SherpaNcnnStream *s) {
  p->recognizer->Reset(s->stream.get());
}

void InputFinished(SherpaNcnnStream *s) { s->stream->InputFinished(); }

int32_t IsEndpoint(SherpaNcnnRecognizer *p, SherpaNcnnStream *s) {
  return p->recognizer->IsEndpoint(s->stream.get());
}

SherpaNcnnDisplay *CreateDisplay(int32_t max_word_per_line) {
  SherpaNcnnDisplay *ans = new SherpaNcnnDisplay;
  ans->impl = std::make_unique<sherpa_ncnn::Display>(max_word_per_line);
  return ans;
}

void DestroyDisplay(SherpaNcnnDisplay *display) { delete display; }

void SherpaNcnnPrint(SherpaNcnnDisplay *display, int32_t idx, const char *s) {
  display->impl->Print(idx, s);
}

}  // namespace asr_api