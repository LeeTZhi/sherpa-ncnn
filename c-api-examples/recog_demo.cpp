/*
 2 threads, one for audio input, one for recognition
 the audio input is simulated from a file
*/

#include <iostream>
#include <fstream>
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

// 函数定义
int findSubstringIndex(const char* str, const char* target) {
    const char* start = str;
    const char* end;
    int index = 0;

    while (*start) {
        end = start;
        // 找到下一个 '\0' 字符
        while (*end && *end != '\0') {
            end++;
        }

        if (strstr(start, target) != NULL) {
            return index;
        }

        // 移动到下一个子字符串的开头
        start = end + 1;
        index++;
    }

    return -1;
}

void audioRecognitionThread(void* recognizer, const std::vector<std::string>& keywords) {
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
                //find the keywords
                for ( int i = 0; i < keywords.size(); i++ ) {
                    int index = findSubstringIndex(results.text, keywords[i].c_str());
                    if ( index != -1 ) {
                        printf("find keyword %s at %d timestamp: %f ms\n", keywords[i].c_str(), index, results.timestamps[index]);
                    }
                }
                printf("Final result: %s\n", results.text);
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

    char* keywords_file = NULL; //所有热词
    char* hotwords_file = NULL; //需要特别关注发音的热词
    if ( argc >= 4 ) {
        keywords_file = argv[3];
    }
    if ( argc >= 5 ) {
        hotwords_file = argv[4];
    }
    config.hotwords_path = hotwords_file;
    config.hotwords_factor = 2.0f;
    
    //load keywords
    std::vector<std::string> keywords;

    if ( keywords_file != NULL ) {
        std::ifstream fin(keywords_file, std::ios::in);
        if ( !fin.is_open() ) {
            fprintf(stderr, "Failed to open %s\n", keywords_file);
            return -1;
        }
        std::string line;
        while ( std::getline(fin, line) ) {
            keywords.push_back(line);
        }

        //print keywords
        for ( int i = 0; i < keywords.size(); i++ ) {
            printf("keyword %d: %s\n", i, keywords[i].c_str());
        }
    }
  
    void *recognizer = CreateStreamASRObject(&config, NULL, 0);
    if ( recognizer == NULL ) {
        fprintf(stderr, "Failed to create recognizer\n");
        return -1;
    }

    const char *wav_filename = argv[2];
  
    ///two threads
    std::thread captureThread(audioCaptureThread, wav_filename);
    std::thread recogThread(audioRecognitionThread, recognizer, keywords);

    captureThread.join();
    recogThread.join();

    DestroyStreamASRObject(recognizer);
    return 0;
}
