//#ifndef AUDIODE_DECODER_H
#define AUDIODE_DECODER_H

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
};

#include<vector>

namespace yymobile
{
    // enum
    // {
    //     FDK_AOT_AAC_LC = 2,
    //     FDK_AOT_SBR = 5,
    //     FDK_AOT_PS = 29
    // };
    
#define AVIO_BUF_SIZE 16384

class CM4a_decoder {
public:
	CM4a_decoder();
	virtual ~CM4a_decoder();

public:
    virtual void stop();
	virtual void start(int codec, int inputFrameSize, int sampleFrequency, int chans, int bps, int profile);
	virtual int  decode(const unsigned char *inputData, unsigned int length, unsigned char* outputData);

    typedef struct AVIOBufferContext {
        unsigned char* ptr;
        int pos;
        int totalSize;
    }AVIOBufferContext;

    int readBuffer(void *opaque, unsigned char *buf, int size)
    {
        AVIOBufferContext* op = (AVIOBufferContext*)opaque;
        if (!op) {
            return -1;
        }
        int len = size;
        if (op->pos + size > op->totalSize) {
            len = op->totalSize - op->pos;
        }
        memcpy(buf, op->ptr + op->pos, len);
        op->pos += len;
        return len;
    }
private:
    
   // int resampleIfNeed(unsigned char* outputData, int len);
	//bool isUseHwDecoder();
    
    //static const int kMaxFrameSizeInByte = 8192 * 2;
    AVPacket packet;
    AVIOBufferContext gAvbufferIn;
    uint8_t* inBuffer        = NULL;
    AVFrame *frame           = NULL;
    AVFrame *frameResampled  = NULL;
    AVCodecContext *codecCtx = NULL;
    AVFormatContext* ifmtCtx = NULL;
    AVIOContext *avioIn      = NULL;
    struct SwrContext *convertCtx = NULL;
    std::vector<uint8_t> resampledBuffer;
	//AudioDecoder *m_decoder;
	//CAdjustVol m_volAdjust;
	//CFileWriter m_fileWriter;
    int m_codec;
    //int m_inputFrameSize;
    int m_insampleRate;
    int m_inchannel;
    int m_bps;
    int m_profile;
	int m_outSampleRate;
	int m_outChannel;
	//SpeexResamplerState* mpResampler;
	//int mLastResamplerChannels;
    //AAC_HWCODEC_MGR_CALLBACK mAacHwCodecMgrCallback;
    //bool mIsUseHwCodec;
	//CSpeexResampler * mStereoReampler;
};

}

//#endif
