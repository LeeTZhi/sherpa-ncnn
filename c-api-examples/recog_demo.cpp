/*
 2 threads, one for audio input, one for recognition
 the audio input is simulated from a file
*/

#include <iostream>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <chrono>
#include <vector>
#include <cstring>
#include <stdio.h>
#include <stdlib.h>

#include "asr_api.h"

std::mutex queueMutex;
std::condition_variable cv;
std::queue<std::vector<int16_t>> audioQueue;  // Queue to store audio chunks
bool recordingDone = false;

void audioCaptureThread(const std::string& filePath) {
    FILE *fp = fopen(filePath.c_str(), "rb");
    if (!fp) {
        fprintf(stderr, "Failed to open %s\n", filePath.c_str());
        return ;
    }
    fseek(fp, 44, SEEK_SET);

    const size_t N = static_cast<size_t>(16000*0.2); //200ms
    int16_t buffer[N];

    while (true) {
        size_t n = fread((void *)buffer, sizeof(int16_t), N, fp);
        if (n > 0) {
            std::vector<int16_t> audioChunk(buffer, buffer + n);
            std::lock_guard<std::mutex> lock(queueMutex);
            audioQueue.push(audioChunk);
            cv.notify_one();
        }
        else { // or reset the header and continue
            std::cerr << "End of file reached" << std::endl;
            {
                std::lock_guard<std::mutex> lock(queueMutex);
                recordingDone = true;
            }
            cv.notify_one(); // Notify after setting the flag
            break;
        }
        ///sleep for 200ms
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    fclose(fp);
}

void audioRecognitionThread(void* recognizer) {
    int32_t segment_id = -1;

    int32_t isFinal = 0;
    int32_t isEnd = 0;
    float sampleRate = 16000.0f;
    ASR_Result results;
    memset(&results, 0, sizeof(results));

    while (!recordingDone ) { 
        std::unique_lock<std::mutex> lock(queueMutex);
        cv.wait(lock, []{ return !audioQueue.empty() || recordingDone; });
        if (audioQueue.empty() && recordingDone) {
            break; // Exit the loop if queue is empty and recording is done
        }

        std::vector<int16_t> audioChunk;
        if(!audioQueue.empty())
        {
            audioChunk = audioQueue.front();
            audioQueue.pop();
            lock.unlock();

            // do recognition
            int ret = StreamRecognize(recognizer, audioChunk.data(), audioChunk.size(), sampleRate, isFinal, &results, &isEnd);
            if ( isEnd && results.text != NULL) {
                printf("End, result: %s\n", results.text);
            }
            DestroyASRResult(&results);
        }
    }
    //padding to recongnize the last chunk
     // add some tail padding
    int16_t data_paddings[8000] = {0};  // 0.5 seconds at 16 kHz sample rate
    StreamRecognize(recognizer, data_paddings, 8000, sampleRate, 1, &results, &isEnd);
    if (results.text != NULL) {
        printf("Final result: %s\n", results.text);
    }
    DestroyASRResult(&results);

    ResetStreamASR(recognizer);
}


int main(int32_t argc, char *argv[]) {
  if (argc < 3 ) {
    fprintf(stderr, "usage: %s\n model.bin wav_path\n", argv[0]);
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
  
    ///two threads
    std::thread captureThread(audioCaptureThread, wav_filename);
    std::thread recogThread(audioRecognitionThread, recognizer);

    captureThread.join();
    recogThread.join();

    DestroyStreamASRObject(recognizer);
    return 0;
}
