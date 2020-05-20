#ifndef __OPUS_ENCODER_H__
#define __OPUS_ENCODER_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <time.h>

#include <iostream>
#include <cassert>
#include <new>

#include "yyaudio.h"
#include "opus.h"
#include "opus_types.h"

#define MAX_PAYLOAD_BYTES		0x3ff

extern "C" {
#include <libavcodec/avcodec.h>
//#include <libavformat/avformat.h>
//#include <libswscale/swscale.h>
//#include <libavutil/imgutils.h>
#include <libswresample/swresample.h>
}

class COpusEncoder : public AudioEncoderBase {
public:
	COpusEncoder(int sampleFrequency, int channels, int bps, int quality);
	virtual ~COpusEncoder();

	virtual int Start();
	virtual void Stop();
	virtual void ClearBuffer() {}
	virtual int  Encode(const unsigned char* inputData, unsigned int length, unsigned char* outputData);
	virtual int  Encode_empty_frame(const unsigned char* inputData, unsigned int length, unsigned char* outputData);
private:
	int m_sampleFrequency;
	int m_channels;
	int m_bps;
	int m_quality;
	int m_inputFrameSize;
	int m_useDTX;
	int m_complexity;

	OpusEncoder *m_enc;

	bool m_isInit;
	struct SwrContext *m_convertCtx;
	int m_encodeSampleFrequency;
	unsigned char *m_resampleBuffer;
	unsigned char m_outputBuffer[MAX_PAYLOAD_BYTES];
};

#endif //__OPUS_ENCODER_H__


