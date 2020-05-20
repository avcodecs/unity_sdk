#ifndef MP3_ENCODER_H
#define MP3_ENCODER_H

#include "AudioEncoder.h"

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavutil/channel_layout.h"
#include "libavutil/common.h"
#include "libavutil/frame.h"
#include "libavutil/samplefmt.h"
};


namespace yymobile
{

class mp3_encoder: public AudioEncoder{
public:
	mp3_encoder():codec(NULL),codecCtx(NULL),frame(NULL),pkt(NULL);
	virtual ~mp3_encoder();

    virtual int Start(int insampleRate, int inchannels, int insampleformat,int bps,int profile,int outsample,int outchannel,int outsampleformat);
    virtual void Stop();
    //     virtual void ClearBuffer() {}
    virtual int  Encode(const unsigned char* inputData, unsigned int len, unsigned char* outputData);
    //     virtual void AdjustEncQuality(int quality);
    // 	virtual void setBitRate(int bitrate);
        
    // 	virtual unsigned int GetEncFinalRange(){return 0;}
    // private:
    //     void resetEncQuality();
private:
    //void* mp3Enc;

    int m_insampleformat;
    int m_inchannel;
    int m_insampleformat;
    int m_bps;
    int m_profile;
    int m_outsample;
    int m_outchannel; 
    int outsampleformat;
    //SKP_SILK_SDK_EncControlStruct m_encControl; // Struct for input to encoder

	//for resample 16k -> 8k
   // webrtc::Resampler *m_resampler;
    //char* m_resampled_data;
   // int m_resample_data_size;
    //audiosdk::ResampleAdapter mResamplerForSilk;


	AVCodec *codec;
    //int outLen;
	AVCodecContext *codecCtx = NULL;
	AVFrame *frame = NULL;
	AVPacket *pkt  = NULL;



};

}

#endif