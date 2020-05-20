#ifndef __OPUS_DECODER_H__
#define __OPUS_DECODER_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <time.h>

#include <iostream>
#include <cassert>
#include <new>

#include "yyaudio.h"

extern "C" {
#include <libavcodec/avcodec.h>
//#include <libavformat/avformat.h>
//#include <libswscale/swscale.h>
//#include <libavutil/imgutils.h>
#include <libswresample/swresample.h>
}


class COpusDecoder : public AudioDecoderBase
{
public:
	COpusDecoder(unsigned int inputFrameSize, int sampleFrequency, int channels, int bps);
	virtual ~COpusDecoder();

	virtual bool Start();
	virtual void Stop();
	virtual void ClearBuffer() {};
	virtual int  Decode(const unsigned char* inputData, unsigned int length, unsigned char* outputData);
	virtual int  Recover(int framenum, unsigned char* outputData);
	virtual int  GetSampleRate(){return m_sampleFrequency;}; 
	virtual int  GetChannels(){return m_channels;}; 
private:
	int m_sampleFrequency;
	int m_channels;
	int m_bps;
	unsigned int m_inputFrameSize;

	AVCodec *m_codec;
	AVCodecContext *m_c;
	int m_lastFrameOutSize;
	AVFrame *m_outputFrame;
	AVPacket *m_pkt;
	struct SwrContext *m_convertCtx;
	int m_decodedSampleFrequency;
	int m_decodedChannels;
	AVSampleFormat m_decodedFormat;
};

#endif //__OPUS_DECODER_H__


