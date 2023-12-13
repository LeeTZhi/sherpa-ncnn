#ifndef __AUDIO_SDK__H__
#define __AUDIO_SDK__H__

#include <list>

#define AI_MAX_CNT         (8)  // max number of ai device created
#define AI_COMBO_MAX_IN    (4)  // support max 2 ai device merge to precess
#define AO_MAX_CNT         (4)  // max number of ao device created
#define ASTREAM_MAX_CNT    (2)  // max number of process pipeline
#define ASTREAM_MAX_NODE   (8)  // max process step of stream
#define ASTREAM_MAX_CHN    (20) // every pipeline can process channels at most
#define ASTREAM_MAX_WORKER (4)
#define AI_FILEPATH_MAX_LEN (40)
/******************************************************************************************************
***************************************  Enum ***************************************
*******************************************************************************************************/

typedef enum tagAUDIO_DEV_TYPE_E
{
    AI_DEV_E_NONE = -1,
    AI_DEV_E_ALSA = 0, // based on tinyalsa
    // AI_DEV_E_UAC   = 1,
    AI_DEV_E_COMBO = 10, // only for AI, merge two AI device input into one
    // AI_DEV_E_WAV_FILE,   // only for AO, dump to wav file
    AI_DEV_E_AFILE = 11,
    AI_DEV_E_BUTT,
} AUDIO_DEV_TYPE_E;

typedef enum tagAI_SAMPLE_RATE_E
{
    AI_SAMPLE_RATE_E_8000  = 8000,  /* 8kHz sampling rate  */
    AI_SAMPLE_RATE_E_16000 = 16000, /* 16kHz sampling rate */
    AI_SAMPLE_RATE_E_32000 = 32000, /* 32kHz sampling rate */
    AI_SAMPLE_RATE_E_48000 = 48000, /* 48kHz sampling rate */
} AI_SAMPLE_RATE_E;

typedef enum tagAENC_TYPE_E
{
    AENC_TYPE_E_UNKNOW = 0,
    AENC_TYPE_E_PCM,
    AENC_TYPE_E_AAC,
    AENC_TYPE_E_G711A,
    AENC_TYPE_E_G711U,
    AENC_TYPE_E_BUTT,
} AENC_TYPE_E;

typedef enum tagAO_WORK_MODE_E
{
    AO_WORK_MODE_E_SYNC,
    // AO_WORK_MODE_E_ASYNC, // not support yet
} AO_WORK_MODE_E;

/******************************************************************************************************
***************************************  Attr of AI  **************************************
*******************************************************************************************************/
struct AUDIO_ALSA_DEV_S {
    int s32Card;
    int s32Device;
};

struct AUDIO_UAC_DEV_S {
    int s32Card;
    int s32Device;
};

struct AUDIO_COMBO_DEV_S {
    int as32AiId[AI_COMBO_MAX_IN];
    int s32AiCnt;
};

struct AUDIO_FILE_DEV_S {
    char acFilePath[AI_FILEPATH_MAX_LEN];
};

union AI_DEVINFO_S {
    AUDIO_ALSA_DEV_S alsa;
    // AUDIO_UAC_DEV_S   uac;
    AUDIO_COMBO_DEV_S combo;
    struct AUDIO_FILE_DEV_S aFile;
};

struct AI_ATTR_S {
    AUDIO_DEV_TYPE_E enDevType;
    AI_DEVINFO_S     stDevInfo;

    bool             bEnabled;
    AI_SAMPLE_RATE_E enSampleRate;
    int              s32Channel;
    int              s32FrameLen; // sample point of pre frame
    int              s32Volume;
};

/******************************************************************************************************
***************************************  Attr of AO  **************************************
*******************************************************************************************************/

union AO_DEVINFO_S {
    AUDIO_ALSA_DEV_S alsa;
    // AUDIO_UAC_DEV_S  uac;
};

struct AO_ATTR_S {
    AUDIO_DEV_TYPE_E enDevType;
    AO_DEVINFO_S     stDevInfo;

    bool          bEnabled;
    AI_SAMPLE_RATE_E enSampleRate;
    int           s32Channel;
    int           s32FrameLen; // sample point of pre frame
    int           s32Volume;

    AO_WORK_MODE_E enWorkMode;
};

/******************************************************************************************************
***************************************  Attr of AI  **************************************
*******************************************************************************************************/

int AUDIO_AI_Create(int s32AiId, AI_ATTR_S *pstAiAttr);
int AUDIO_AI_Destroy(int s32AiId);
int AUDIO_AI_GetAttr(int s32AiId, AI_ATTR_S *pstAiAttr);
int AUDIO_AI_SetAttr(int s32AiId, AI_ATTR_S *pstAiAttr);
int AUDIO_AI_SetVolume(int s32AiId, int s32Volume);
int AUDIO_AI_SetEnable(int s32AiId, bool bEnable);

int AUDIO_AI_Open(int s32AiId);
int AUDIO_AI_Close(int s32AiId);

int AUDIO_AI_Read(int       s32AiId,
                     unsigned char  *buffer,
                     unsigned int  u32BufLen,
                     unsigned int &u32ReadLen,
                     unsigned long long &pts,
                     int  s32TimeoutMs);

bool AUDIO_AI_isEnabled(int s32AiId);
bool AUDIO_AI_isInitd(int s32AiId);
bool AUDIO_AI_isOpen(int s32AiId);
int  AUDIO_AI_GetChannelChn(int s32AiId);
int  AUDIO_AI_GetRecommendReadBufferSize(int s32AiId);

/******************************************************************************************************
***************************************  Attr of AO  **************************************
*******************************************************************************************************/

int AUDIO_AO_GetNewId();
int AUDIO_AO_Create(int s32AoId, AO_ATTR_S *pstAoAttr);
int AUDIO_AO_Destroy(int s32AoId);
int AUDIO_AO_GetAttr(int s32AoId, AO_ATTR_S *pstAoAttr);
int AUDIO_AO_SetAttr(int s32AoId, AO_ATTR_S *pstAoAttr);
int AUDIO_AO_SetVolume(int s32AoId, int s32Volume);
int AUDIO_AO_SetEnable(int s32AoId, bool bEnable);

bool AUDIO_AO_isEnabled(int s32AoId);
bool AUDIO_AO_isReady(int s32AoId);

int AUDIO_AO_Write(int s32AoId, unsigned char *buffer, unsigned int u32BufLen);

#endif // __AUDIO_SDK__H__
