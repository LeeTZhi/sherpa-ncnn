#include <iostream>
#include <signal.h>

#include "audio_sdk.h"

#define PCM_FILE "/var/pcm_save"
#define PCM_LEN_MAX (1024 * 1024)

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
    stAiAttr.s32Volume = 32;
    stAiAttr.bEnabled = true;

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

    unsigned int buffer_len = AUDIO_AI_GetRecommendReadBufferSize(s32AiId);
    unsigned int actual_len = 0;
    unsigned int total_len  = 0;
    unsigned char buffer[buffer_len] = {0};
    unsigned long long pts;

    while(!g_exit)
    {
        ret = AUDIO_AI_Read(s32AiId, buffer, buffer_len, actual_len, pts, 0);
        if (ret != 0) {
            printf("read AI[%d] failed\n", s32AiId);
            break;
        }
        if (total_len < PCM_LEN_MAX) {
            fwrite(buffer, 1, buffer_len, fp);
        }
        else {
            break;
        }
        total_len += buffer_len;
    }

    fclose(fp);
    AUDIO_AI_Close(s32AiId);

    return 0;
}
