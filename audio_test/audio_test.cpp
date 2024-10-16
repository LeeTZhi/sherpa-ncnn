#include <iostream>
#include <fstream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <vector>

#include <signal.h>

#include "audio_sdk.h"

#define PCM_FILE "/var/pcm_save"
#define PCM_LEN_MAX (1024 * 1024)

typedef struct {
    int s32AiId;
    int buffer_len;
} Capture_info;

std::queue<std::vector<uint8_t>> g_queue;
std::mutex g_mutex;
std::condition_variable cv;
bool recordingDone = false;

void Capture_thread(void *info_in);
void audioWriteThread(const std::string& filename);

int g_exit = 0;
void  _Exit(int signo)
{
    exit(0);
}

int main()
{
    int s32AiId = 0;
    AI_ATTR_S stAiAttr;
    int ret;

    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT ,_Exit);
    signal(SIGTERM,_Exit);

    stAiAttr.enDevType = AI_DEV_E_ALSA;
    stAiAttr.stDevInfo.alsa.s32Card = 2;//MicIn
    stAiAttr.stDevInfo.alsa.s32Device = 0;
    stAiAttr.enSampleRate = AI_SAMPLE_RATE_E_16000;
    stAiAttr.s32Channel = 2;
    stAiAttr.s32FrameLen = 1024;
    stAiAttr.s32Volume = 8;
    stAiAttr.bEnabled = false;

    AUDIO_AI_Create(s32AiId, &stAiAttr);

    ret = AUDIO_AI_SetEnable(s32AiId, true);
    if (ret != 0) {
        printf("enable AI[%d] failed\n", s32AiId);
        return -1;
    }

    ret = AUDIO_AI_Open(s32AiId);
    if (ret != 0) {
        printf("open AI[%d] failed\n", s32AiId);
    }

    FILE *fp;
    fp = fopen(PCM_FILE, "wb+");
    if (!fp) {
        AUDIO_AI_Close(s32AiId);
        return -1;
    }

    Capture_info info;
    info.s32AiId = s32AiId;
    info.buffer_len = AUDIO_AI_GetRecommendReadBufferSize(s32AiId);
    
    std::thread captureThread(Capture_thread, &info);
    std::thread writeThread(audioWriteThread, "output.pcm");

    // Some code to handle recording termination

    captureThread.join();
    writeThread.join();

    
    AUDIO_AI_Close(s32AiId);
    return 0;
}

void Capture_thread(void *info_in)
{
    Capture_info *info = (Capture_info *)info_in;
    int s32AiId = info->s32AiId;
    unsigned int buffer_len = info->buffer_len;
    int ret;
    unsigned int actual_len = 0;
    unsigned int total_len  = 0;
    unsigned char* buffer = new unsigned char[buffer_len];
    unsigned long long pts;

    while(!recordingDone)
    {
        ret = AUDIO_AI_Read(s32AiId, buffer, buffer_len, actual_len, pts, 0);
        if (ret != 0) {
            printf("read AI[%d] failed\n", s32AiId);
            break;
        }
        if (actual_len > 0) {
            std::vector<uint8_t> pcm(actual_len);
            memcpy(pcm.data(), buffer, actual_len);
            std::unique_lock<std::mutex> lock(g_mutex);
            g_queue.push(pcm);
            cv.notify_one();
        }
    }
    recordingDone = true;
    delete[] buffer;
}

void audioWriteThread(const std::string& filename) {
    // Open file
    std::ofstream outFile(filename, std::ios::binary);
    while (!recordingDone || !g_queue.empty()) {
        std::unique_lock<std::mutex> lock(g_mutex);
        cv.wait(lock, []{ return !g_queue.empty() || recordingDone; });
        while (!g_queue.empty()) {
            auto frame = g_queue.front();
            g_queue.pop();
            lock.unlock();
            // Write frame to file
            outFile.write(reinterpret_cast<const char*>(frame.data()), frame.size());
            lock.lock();
        }
    }
    outFile.close();
}