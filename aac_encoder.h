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
	mp3_encoder(int sampleFrequency, int channels, int bps, int bitrate);
	virtual ~mp3_encoder();

    virtual int Start();
    virtual void Stop();
    //     virtual void ClearBuffer() {}
    //     virtual int  Encode(const unsigned char* inputData, unsigned int len, unsigned char* outputData);
    //     virtual void AdjustEncQuality(int quality);
    // 	virtual void setBitRate(int bitrate);
        
    // 	virtual unsigned int GetEncFinalRange(){return 0;}
    // private:
    //     void resetEncQuality();
private:
    void* mp3Enc;

    int m_sampleFrequency;
    int m_channels;
    int m_bps;       
	int m_bitrate;
    volatile int m_quality;
    int m_lastQuality;
    int m_inputFrameSize;
  	int m_useDTX;
  	int m_complexity;
    int m_EncCount;
    SKP_SILK_SDK_EncControlStruct m_encControl; // Struct for input to encoder

	//for resample 16k -> 8k
    webrtc::Resampler *m_resampler;
    char* m_resampled_data;
    int m_resample_data_size;
    audiosdk::ResampleAdapter mResamplerForSilk;
};

}