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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sherpa-ncnn/c-api/asr_api.h"

const char *kUsage =
    "\n"
    "Usage:\n"
    "  ./bin/decode-file-c-api \\\n"
    "    /path/to/tokens.txt \\\n"
    "    /path/to/encoder.ncnn.param \\\n"
    "    /path/to/encoder.ncnn.bin \\\n"
    "    /path/to/decoder.ncnn.param \\\n"
    "    /path/to/decoder.ncnn.bin \\\n"
    "    /path/to/joiner.ncnn.param \\\n"
    "    /path/to/joiner.ncnn.bin \\\n"
    "    /path/to/foo.wav [<num_threads> [decode_method, can be "
    "greedy_search/modified_beam_search]]"
    "\n\n"
    "Please refer to \n"
    "https://k2-fsa.github.io/sherpa/ncnn/pretrained_models/index.html\n"
    "for a list of pre-trained models to download.\n";

int32_t main(int32_t argc, char *argv[]) {
  if (argc < 9 || argc > 13) {
    fprintf(stderr, "%s\n", kUsage);
    return -1;
  }

  ASR_Parameters config;
  memset(&config, 0, sizeof(config));

  config.version = FAST;
  config.faster_model_name = argv[1];

  int32_t num_threads = 4;
  if (argc >= 10 && atoi(argv[9]) > 0) {
    num_threads = atoi(argv[9]);
  }




  const char *wav_filename = argv[8];
  FILE *fp = fopen(wav_filename, "rb");
  if (!fp) {
    fprintf(stderr, "Failed to open %s\n", wav_filename);
    return -1;
  }

  // Assume the wave header occupies 44 bytes.
  fseek(fp, 44, SEEK_SET);

  // simulate streaming

#define N 3200  // 0.2 s. Sample rate is fixed to 16 kHz

  int16_t buffer[N];
  float samples[N];
  SherpaNcnnStream *s = CreateStream(recognizer);

  SherpaNcnnDisplay *display = CreateDisplay(50);
  int32_t segment_id = -1;

  while (!feof(fp)) {
    size_t n = fread((void *)buffer, sizeof(int16_t), N, fp);
    if (n > 0) {
      for (size_t i = 0; i != n; ++i) {
        samples[i] = buffer[i] / 32768.;
      }
      AcceptWaveform(s, 16000, samples, n);
      while (IsReady(recognizer, s)) {
        Decode(recognizer, s);
      }

      SherpaNcnnResult *r = GetResult(recognizer, s);
      if (strlen(r->text)) {
        SherpaNcnnPrint(display, segment_id, r->text);
      }
      DestroyResult(r);
    }
  }
  fclose(fp);

  // add some tail padding
  float tail_paddings[4800] = {0};  // 0.3 seconds at 16 kHz sample rate
  AcceptWaveform(s, 16000, tail_paddings, 4800);

  InputFinished(s);

  while (IsReady(recognizer, s)) {
    Decode(recognizer, s);
  }
  SherpaNcnnResult *r = GetResult(recognizer, s);
  if (strlen(r->text)) {
    SherpaNcnnPrint(display, segment_id, r->text);
  }

  DestroyResult(r);

  DestroyDisplay(display);

  DestroyStream(s);
  DestroyRecognizer(recognizer);

  fprintf(stderr, "\n");

  return 0;
}
