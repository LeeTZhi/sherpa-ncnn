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
#include <sys/time.h>

#include "sherpa-ncnn/c-api/asr_api.h"

const char *kUsage =
    "\n"
    "Usage:\n"
    "  ./bin/decode-file-asr \\\n"
    "    /path/to/data.bin \\\n"
    "    /path/to/foo.wav \n";

int32_t main(int32_t argc, char *argv[]) {
  if (argc < 3 ) {
    fprintf(stderr, "%s\n", kUsage);
    return -1;
  }
  ASR_Parameters config;
  memset(&config, 0, sizeof(config));

  config.version = FAST;
  config.faster_model_name = argv[1];
  config.enable_endpoint = 1;
  config.rule1_min_threshold = 2.5f;
  config.rule2_min_threshold = 1.2f;
  config.rule3_min_threshold = 120.0f;
  
  
  void *recognizer = CreateStreamASRObject(&config, NULL, 0);
  if ( recognizer == NULL ) {
    fprintf(stderr, "Failed to create recognizer\n");
    return -1;
  }

  const char *wav_filename = argv[2];
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
  
  int32_t segment_id = -1;

  int32_t isFinal = 0;
  int32_t isEnd = 0;
  float sampleRate = 16000.0f;
  ASR_Result results;
  memset(&results, 0, sizeof(results));
  int count = 0;
  ///define the total elapsed time
  struct timeval start, end;
  long milseconds, useconds;    
  double total_elapsed_time = 0;

  while (!feof(fp)) {
    size_t n = fread((void *)buffer, sizeof(int16_t), N, fp);
    if (n > 0) {
      ///t0
      gettimeofday(&start, NULL);
      int ret = StreamRecognize(recognizer, buffer, n, sampleRate, isFinal, &results, &isEnd);
      gettimeofday(&end, NULL);
      milseconds  = (end.tv_sec  - start.tv_sec)*1000;
      useconds   = (end.tv_usec - start.tv_usec)/1000;
      total_elapsed_time += (milseconds + useconds/1000.0);

      if (ret != 0 ) {
        fprintf(stderr, "Failed to recognize\n");
        return -1;
      }
      printf("Results: %s\n", results.text);
      if (isEnd ){
        printf("isEnd\n");
      }
      DestroyASRResult(&results);
    }
    printf("frame count: %d\n", count++);
  }
  fclose(fp);

  // add some tail padding
  int16_t tail_paddings[4800] = {0};  // 0.3 seconds at 16 kHz sample rate
  StreamRecognize(recognizer, tail_paddings, 4800, sampleRate, 1, &results, &isEnd);
  

  
  ResetStreamASR(recognizer);

  fprintf(stderr, "\n");
  fprintf(stderr, "Total elapsed time: %f ms average: %f ms\n", total_elapsed_time, total_elapsed_time/count);
  return 0;
}
